/*
 * TextScroll — hi-res dual-playfield scroller with per-line BPLxPT and CPU font stamp.
 *
 * Demoscene / hardware picture:
 * - Display is HIRES 640×256 with MODE_DUALPF: one playfield is an 8×8 font
 *   scroller (single bitplane in `scroll->planes[0]`), the other is the static
 *   background (`scroll->planes[1]` points at `_background_bpl` from `background.c`).
 *   Colours come from two palettes: `font_colors` (indices 0–7) and `background_colors`
 *   (loaded at index 8+).
 * - Vertical motion is not done by blitting the whole screen: each raster line has
 *   its own Copper MOVE to `BPL1PT` pointing into a tall off-screen buffer (`scroll` is
 *   HEIGHT+16 lines). `SetupLinePointers` rotates those pointers with a phase derived
 *   from `frameCount` so the text bitmap appears to roll upward — a “rolling buffer”
 *   / line-pointer scroll (see HRM: bitplane pointers, Copper lists).
 * - New text lines are drawn occasionally with RenderLine: the CPU copies 8-pixel
 *   vertical strips from the 1bpp font into one horizontal row of the bitmap, then
 *   `BitmapClearArea` clears that band before drawing — no font blitter path, plain
 *   byte writes (fast enough for one row per few frames).
 * - Top/bottom gradient on colour 1: per block of 8 lines, CopSetColor adjusts
 *   the pen used for the font playfield for a simple border glow.
 *
 * Why not one BPLCON1 scroll: fine horizontal scroll is unused here; the effect is
 * vertical storytelling + dual-PF parallax, not a horizontal sine.
 *
 * Why two identical copper lists (`cp[0]`, `cp[1]`): each frame rewrites every line’s
 * BPL1PT. Alternating `active` means we always fill the list that was *not* bound on
 * the previous `CopListRun`, so the Copper is never mid-stream through a list while
 * its MOVEs are being overwritten. `CopListRun` after patching arms that buffer for
 * the next field; `TaskWaitVBlank` + `active ^= 1` complete the hand-off.
 *
 * HRM: https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * HRM mirror: http://amigadev.elowar.com/read/
 */
#include "custom_regdef.h"
#include <effect.h>
#include <blitter.h>
#include <copper.h>
#include <gfx.h>
#include <system/memory.h>

/* Hires playfield width; must match font/background asset layout. */
#define WIDTH 640
#define HEIGHT 256
/* Font + scroll plane depth (background depth added in SetupMode). */
#define DEPTH 1

#define SIZE 8
#define SPEED 1

#define COLUMNS (WIDTH / SIZE)
#define LINES   (HEIGHT / SIZE)

static __code short active = 0;

static __code CopListT *cp[2];
/* linebpl[buffer][line] — CopIns to patch BPL1PT for that display line. */
static __code CopInsPairT *(*linebpl)[2][HEIGHT];
/* Tall bitmap: plane 0 = scroll, plane 1 = shared background CHIP. */
static __code BitmapT *scroll;

static __code short last_line = -1;
static __code char *line_start;

extern char Text[];

#include "data/text-scroll-font.c"
#include "data/background.c"

/*
 * Build copper list: set up both planes via `CopSetupBitplanes`, then for each
 * scanline WAIT + record `CopMove32(bplpt[0], rowptr)` so `SetupLinePointers` can
 * retarget each line’s fetch into the rolling vertical window.
 */
static CopListT *MakeCopperList(CopInsPairT **linebpl) {
  CopListT *cp = NewCopList(100 + 3 * HEIGHT);
  CopSetupBitplanes(cp, scroll, DEPTH + background_depth);
  {
    void *ptr = scroll->planes[0];
    short y;

    for (y = 0; y < HEIGHT; y++, ptr += scroll->bytesPerRow) {
      CopWaitSafe(cp, Y(y), HP(0));
      /* Every 8 lines, nudge colour 1 for a soft gradient at top and bottom strips. */
      if ((y & 7) == 0) {
        if (y <= 6 * 8)
          CopSetColor(cp, 1, font_colors[7 - (y >> 3)]);
        if (HEIGHT - y <= 6 * 8)
          CopSetColor(cp, 1, font_colors[7 - ((HEIGHT - y) >> 3)]);
      }
      linebpl[y] = CopMove32(cp, bplpt[0], ptr);
    }
  }
  return CopListFinish(cp);
}

/* Allocate tall scroll buffer, attach background plane, dual-PF hi-res, double copper. */
static void Init(void) {
  scroll = NewBitmap(WIDTH, HEIGHT + 16, 1, BM_CLEAR);
  scroll->planes[1] = _background_bpl;

  line_start = Text;

  SetupDisplayWindow(MODE_HIRES, X(0), Y(0), WIDTH, HEIGHT);
  SetupBitplaneFetch(MODE_HIRES, X(0), WIDTH);
  SetupMode(MODE_DUALPF|MODE_HIRES, DEPTH + background_depth);
  LoadColors(font_colors, 0);
  LoadColors(background_colors, 8);

  linebpl = MemAlloc(sizeof(CopInsPairT *) * 2 * HEIGHT, MEMF_PUBLIC);
  cp[0] = MakeCopperList((*linebpl)[0]);
  cp[1] = MakeCopperList((*linebpl)[1]);

  CopListActivate(cp[active]);

  EnableDMA(DMAF_RASTER|DMAF_BLITTER);
}

static void Kill(void) {
  /* Stop copper before freeing lists; BlitterStop in case clear/blit was mid-flight. */
  CopperStop();
  BlitterStop();

  DeleteCopList(cp[0]);
  DeleteCopList(cp[1]);
  MemFree(linebpl);
  DeleteBitmap(scroll);
}

/*
 * Stamp one text row. The font asset stores each glyph as eight rows of one byte
 * (one row of 8 pixels per byte in `font.planes[0]`). For each character we walk
 * those eight rows and write into eight consecutive scanlines of the scroll bitmap
 * at the same column — that matches how the display will read the tall buffer line
 * by line. A blitter font blit would work but costs setup; one row at a time keeps
 * CPU time predictable for the scroller.
 */
static void RenderLine(u_char *dst, char *line, short size) {
  short dwidth = scroll->bytesPerRow;
  short swidth = font.bytesPerRow;
  u_char *src = font.planes[0];
  short x = 0;

  while (--size >= 0) {
    short i = (*line++) - 32;
    short j = x++;
    short h = 8;

    if (i < 0)
      continue;

    while (--h >= 0) {
      dst[j] = src[i];
      i += swidth;
      j += dwidth;
    }
  }
}

/*
 * Patch every line’s BPL1PT so each raster line fetches from a different row of the
 * tall CHIP buffer. Agnus does not “scroll” memory: it always reads one row per
 * display line from the address in BPLxPT. By giving line 0 the address of row R,
 * line 1 row R+1, … with wrap at buffer end, the eye sees text moving up even though
 * we only advance `start` by one row’s worth of `frameCount` phase — no full-frame
 * memcpy. The `+ 8` phase offsets where the window starts so content isn’t stuck
 * at the top edge.
 */
static void SetupLinePointers(void) {
  CopInsPairT **ins = (*linebpl)[active];
  void *plane = scroll->planes[0];
  int stride = scroll->bytesPerRow;
  int bplsize = scroll->bplSize;
  short y = (int)(frameCount / SPEED + 8) % (short)scroll->height;
  void *start = plane + (short)stride * y;
  void *end = plane + bplsize;
  short n = HEIGHT;

  while (--n >= 0) {
    if (start >= end)
      start -= bplsize;
    CopInsSet32(*ins++, start);
    start += stride;
  }
}

/* Return pointer after next newline, or end of string if none. */
static char *NextLine(char *str) {
  for (; *str; str++)
    if (*str == '\n')
      return ++str;
  return str;
}

/*
 * Decouple scroll speed from text feed: only every `SPEED*8` frames do we consider
 * drawing another source line — roughly once per “row” of 8px cells so new glyphs
 * appear in the band that is about to scroll into view, without fighting the
 * per-frame pointer walk in SetupLinePointers.
 */
static void RenderNextLineIfNeeded(void) {
  Area2D rect = {0, 0, WIDTH, SIZE};
  short s = frameCount / (SPEED * 8);

  if (s > last_line) {
    void *ptr = scroll->planes[0];
    short line_num = (s % (LINES + 2)) * SIZE;
    char *line_end;
    short size;

    line_end = NextLine(line_start);
    size = (line_end - line_start) - 1;

    ptr += line_num * scroll->bytesPerRow;

    rect.y = line_num;
    BitmapClearArea(scroll, &rect);
    WaitBlitter();
    RenderLine(ptr, line_start, min(size, COLUMNS));

    last_line = s;
    line_start = line_end;
  }
}

/*
 * 1) Retarget all line MOVEs in `cp[active]`.
 * 2) Optionally rasterize the next string line into the tall bitmap.
 * 3) Arm COP1LC to that list (visible next time the copper restarts its list fetch).
 * 4) Wait for beam, then flip `active` so the next frame rewrites the other list.
 */
static void Render(void) {
  SetupLinePointers();
  RenderNextLineIfNeeded();

  CopListRun(cp[active]);
  TaskWaitVBlank();
  active ^= 1;
}

EFFECT(TextScroll, NULL, NULL, Init, Kill, Render, NULL);
