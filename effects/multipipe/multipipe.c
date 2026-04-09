/*
 * MultiPipe — fake “tunnel” of stripes: precomputed CHIP lines + per-line copper patch.
 *
 * `CalcLines` fills `cache[STEP][…]` with horizontal 1bpp patterns for each tunnel
 * width (sawtooth phase) and sub-pixel phase (`STEP` discrete shifts). At runtime
 * `RenderPipes` only picks which pre-baked row to show and a BPLCON1 nibble — no
 * per-pixel blitter on the tunnel itself. `offsets[]` maps each screen line’s stripe
 * width (`stripes` from stripes.c) to a byte offset inside the cache so the same
 * pattern templates scale with perspective.
 *
 * Two copper lists (`cp[0]`, `cp[1]`) alternate like `textscroll`: patch all
 * `CopLineT` per frame, `CopListRun`, VBlank, `active ^= 1`. `SetupBitplaneFetch` uses
 * extra margin (X(-16), WIDTH+16) so BPLCON1 fine shifts do not eat visible pixels.
 *
 * HRM: https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * HRM mirror: http://amigadev.elowar.com/read/
 */

#include <effect.h>
#include <blitter.h>
#include <copper.h>
#include <gfx.h>
#include <pixmap.h>
#include <system/memory.h>

#define WIDTH 320
#define HEIGHT 256
#define DEPTH 2

typedef struct {
  CopInsT bplshift;
  CopInsPairT bplptr0;
  CopInsPairT bplptr1;
  CopInsT color0;
  CopInsT color1;
} CopLineT;

static CopListT *cp[2];
static CopLineT *copLines[2][HEIGHT];
static short active = 1;

#include "stripes.c"
#include "data/colors.c"

#define STEP_SHIFT 3
#define STEP (1 << STEP_SHIFT)
#define MIN_W (int)(21.0 * STEP)
#define MAX_W (int)(37.0 * STEP)
#define FULL (MAX_W - MIN_W + 1)
#define EMPTY (MAX_W - MIN_W + 2)
#define CHEIGHT (MAX_W - MIN_W + 3)
#define CWIDTH (WIDTH + 64)

static u_char *cache[STEP];
static int offsets[HEIGHT];

static u_short shifts[16] = {
  0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88,
  0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00,
};

/*
 * Build once: for each horizontal phase `s`, generate all stripe-width variants
 * (toggle bit when accumulated phase crosses `w`) plus a solid border line.
 */
void CalcLines(void) {
  short s;

  for (s = 0; s < STEP; s++) {
    u_char *line = cache[s];

    short i, j;

    for (j = 0; j <= (MAX_W - MIN_W); j++) {
      short w = MIN_W + j;
      short x = s;
      short c = 0;

      for (i = 0; i < CWIDTH; i++) {
        if (c)
          bset(line + (i >> 3), 7 - (i & 7));
        x += STEP;
        if (x > w) {
          x -= w;
          c ^= 1;
        }
      }

      line += CWIDTH / 8;
    }

    for (i = 0; i < CWIDTH; i++) {
      bset(line + (i >> 3), 7 - (i & 7));
    }
  }

  {
    int *offset = offsets;
    short *stripe = stripes;
    short y;

    for (y = 0; y < HEIGHT; y++) {
      short w = *stripe++;
      *offset++ = (w - MIN_W) * (short)(CWIDTH / 8);
    }
  }
}

static void Load(void) {
  short s;

  for (s = 0; s < STEP; s++)
    cache[s] = MemAlloc(CWIDTH * CHEIGHT / 8, MEMF_CHIP | MEMF_CLEAR);
}

static void UnLoad(void) {
  short s;

  for (s = 0; s < STEP; s++) 
    MemFree(cache[s]);
}

static CopListT *MakeCopperList(CopLineT **line) {
  CopListT *cp = NewCopList(50 + HEIGHT * 8);
  void *data = cache[0];
  short i;

  CopSetColor(cp, 0, 0x000);
  CopSetColor(cp, 1, 0x000);
  for (i = 0; i < HEIGHT; i++) {
    CopWaitSafe(cp, Y(i), HP(0));
    line[i] = (CopLineT *)CopMove16(cp, bplcon1, 0x00);
    CopMove32(cp, bplpt[0], data + EMPTY * CWIDTH / 8);
    CopMove32(cp, bplpt[1], data + FULL * CWIDTH / 8);
    CopSetColor(cp, 2, 0x000);
    CopSetColor(cp, 3, 0x000);
  }
  return CopListFinish(cp);
}

static void Init(void) {
  CalcLines();

  SetupMode(MODE_LORES, DEPTH);
  SetupDisplayWindow(MODE_LORES, X(0), Y(0), WIDTH, HEIGHT);
  SetupBitplaneFetch(MODE_LORES, X(-16), WIDTH + 16);

  cp[0] = MakeCopperList(copLines[0]);
  cp[1] = MakeCopperList(copLines[1]);
  CopListActivate(cp[0]);
  EnableDMA(DMAF_RASTER);
}

static void Kill(void) {
  DisableDMA(DMAF_RASTER);
  DeleteCopList(cp[0]);
  DeleteCopList(cp[1]);
}

static void RenderPipes(void) {
  u_short *pixels = (u_short *)colors.pixels;
  CopLineT **lineTab = copLines[active];
  int *offset = offsets;
  short *stripe = stripes;
  short i;
  register short frame = frameCount;
  register int center asm("d7") = - WIDTH * STEP / 2;

  for (i = 0; i < HEIGHT; i++) {
    CopLineT *lineIns = *lineTab++;
    short w = *stripe++;
    int x = ((short)w * (short)frame) >> 3;
    short _x, _c;

    x += center;

#if 0
    _x = mod16(x, w);
    _c = div16(x, w);
#else
    asm("divs %1,%0" : "+d" (x) : "d" (w));
    _c = x;
    _x = swap16(x);
#endif

    if (_x < 0) {
      _x += w;
      _c += 1;
    }

    /* Point BPLxPT at the right row inside `cache[phase]` + per-line width table. */
    {
      void *line;
      short off;

      line = (void *)getlong((void *)cache, _x & (STEP - 1));
      line += *offset++;

      _x = _x >> STEP_SHIFT;

      off = (_x >> 3) & ~1;
#if 0
      line += off;
#else
      asm("extl %1\n"
          "addl %1,%0"
          : "+d" (line) : "d" (off) : "1");
#endif
      CopInsSet32(&lineIns->bplptr0, line);
    }

    /* Table lookup avoids minterm math on a 68000 in the inner loop. */
    CopInsSet16(&lineIns->bplshift, getword(shifts, _x & 15));

    /* Alternate band colours when crossing stripe boundaries (parity of `_c`). */
    {
      short c0 = *pixels++;
      short c1 = *pixels++;

      if (_c & 1) swapr(c0, c1);

      CopInsSet16(&lineIns->color0, c0);
      CopInsSet16(&lineIns->color1, c1);
    }
  }
}

PROFILE(RenderPipes);

static void Render(void) {
  ProfilerStart(RenderPipes);
  {
    RenderPipes();
  }
  ProfilerStop(RenderPipes); 

  CopListRun(cp[active]);
  TaskWaitVBlank();
  active ^= 1;
}

EFFECT(MultiPipe, Load, UnLoad, Init, Kill, Render, NULL);
