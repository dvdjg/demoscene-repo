/*
 * Neons — vertical greet scroller with “neon” masks on extra bitplanes + palette rotation.
 *
 * Display: 5 bitplanes (32 colours). Logos are 4bpp art; planes 3–4 are filled with
 * BlitterSetArea to carve bright outlines (colour index 0 vs 1 in GreetingT selects
 * minterm / fill pattern). Background is restored from `background` where each logo
 * moved last frame (`ClearCliparts` blits a thin strip of the previous position then
 * full background patch for the dirty rect).
 *
 * Scroll: each frame, greeting positions move up by `step` proportional to
 * `(frameCount - lastFrameCount)` (see effect.h) so speed tracks real frame delta.
 *
 * VBlank hook `CustomRotatePalette`: patches only 15 copper MOVEs (skips pen 0) so
 * the sky/background base pen stays stable while pens 1–14 cycle — avoids flicker
 * on the border colour that anchors the composition.
 *
 * Why blitter clear + copy for cleanup: logos move faster than one pixel per frame.
 * Clearing the entire old bounding box every time would cost more DMA; we only
 * BlitterClearArea the top-most 8 scanlines of the previous stamp (where the new
 * position left “exposed” background) then BitmapCopyArea the same sliver from the
 * clean background asset. The rest of the old rect is overwritten by the new draw.
 *
 * HRM: https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * HRM mirror: http://amigadev.elowar.com/read/
 */

#include "effect.h"
#include "copper.h"
#include "gfx.h"
#include "blitter.h"
#include "2d.h"
#include "fx.h"
#include <stdlib.h>

#define WIDTH 320
#define HEIGHT 256
#define DEPTH 5

#define PNUM 19

static BitmapT *screen[2];
static u_short active = 0;
static CopInsPairT *bplptr;
static CopListT *cp;
/* First CopIns of CopLoadColors(background) — CustomRotatePalette patches MOVE data. */
static CopInsT *pal;

#include "data/greet_ada.c"
#include "data/greet_blacksun.c"
#include "data/greet_dcs.c"
#include "data/greet_dekadence.c"
#include "data/greet_desire.c"
#include "data/greet_dinx.c"
#include "data/greet_elude.c"
#include "data/greet_fd.c"
#include "data/greet_floppy.c"
#include "data/greet_lemon.c"
#include "data/greet_loonies.c"
#include "data/greet_moods.c"
#include "data/greet_nah.c"
#include "data/greet_rno.c"
#include "data/greet_skarla.c"
#include "data/greet_speccy.c"
#include "data/greet_tulou.c"
#include "data/greet_wanted.c"
#include "data/greet_ycrew.c"
#include "data/neons.c"

/* Per-greeting visible band last frame — used to clear before redraw. */
static Area2D grt_area[2][PNUM];

typedef struct {
  short color;
  const BitmapT *bitmap;
  Point2D pos;
} GreetingT;

/* color: 0/1 selects neon fill variant on planes 3–4 (see DrawCliparts). */
#define GREETING(color, group) {(color), &(greet_ ## group), {0, 0}}

static GreetingT greeting[PNUM] = {
  GREETING(0, ada),
  GREETING(0, blacksun),
  GREETING(1, dcs),
  GREETING(0, dekadence),
  GREETING(1, desire),
  GREETING(0, dinx),
  GREETING(1, elude),
  GREETING(0, fd),
  GREETING(1, floppy),
  GREETING(0, lemon),
  GREETING(1, loonies),
  GREETING(1, moods),
  GREETING(0, nah),
  GREETING(0, rno),
  GREETING(1, skarla),
  GREETING(0, speccy),
  GREETING(0, tulou),
  GREETING(1, wanted),
  GREETING(1, ycrew)
};

/* Stagger greet logos vertically below the screen; odd/even columns alternate X. */
static void PositionGreetings(void) {
  GreetingT *grt = greeting;
  short y = HEIGHT + 200;
  short i;
  
  for (i = 0; i < PNUM; i++) {
    Point2D *pos = &grt->pos;
    const BitmapT *src = grt->bitmap;

    pos->x = (i & 1) ? (WIDTH / 2 - 64) : (WIDTH / 2 + 64 - src->width);
    pos->y = y;

    y += src->height / 2 + (random() & 31) + 10;

    grt++;
  }
}

static void Load(void) {
  screen[0] = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR);
  screen[1] = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR);

  PositionGreetings();
}

static void UnLoad(void) {
  DeleteBitmap(screen[0]);
  DeleteBitmap(screen[1]);
}

/*
 * Walk the 15 MOVE instructions after the first colour load: pen 0 (first CopIns from
 * CopLoadColors) is left alone; pens 1..14 get rotating RGB from background_colors.
 * CopInsSet16 hits only `move.data`, so register addresses in the list stay valid.
 */
static void CustomRotatePalette(void) {
  u_short *src = background_colors;
  CopInsT *ins = pal + 1;
  int i = frameCount;
  short n = 15;

  while (--n >= 0)
    CopInsSet16(ins++, src[i++ & 15]);

  return;
}

static CopListT *MakeCopperList(void) {
  CopListT *cp = NewCopList(100);
  bplptr = CopSetupBitplanes(cp, screen[active], DEPTH);
  pal = CopLoadColors(cp, background_colors, 0);
  CopLoadColors(cp, moods_colors, 16);
  CopLoadColors(cp, rno_colors, 24);
  return CopListFinish(cp);
}

static void Init(void) {
  EnableDMA(DMAF_BLITTER);

  BitmapCopy(screen[0], 0, 0, &background);
  BitmapCopy(screen[1], 0, 0, &background);

  /* Top plane cleared — neon layers built by DrawCliparts each frame. */
  BlitterClear(screen[0], 4);
  BlitterClear(screen[1], 4);

  SetupPlayfield(MODE_LORES, DEPTH, X(0), Y(0), WIDTH, HEIGHT);

  cp = MakeCopperList();
  CopListActivate(cp);
  EnableDMA(DMAF_RASTER);
}

static void Kill(void) {
  DisableDMA(DMAF_COPPER | DMAF_RASTER | DMAF_BLITTER);

  DeleteCopList(cp);
}

/*
 * grt_area holds the *previous* visible slice. Logos move upward; the new garbage is
 * a thin strip at the bottom of the old bitmap. Clamp to 8px height so we never clear
 * more than one band per logo per frame — enough when step ≤ 8-ish; avoids full-area
 * clears. Plane 4 cleared first so neon mask bits don’t smear; then background art
 * restores colour planes for that strip.
 */
static void ClearCliparts(void) {
  Area2D *area = grt_area[active];
  BitmapT *dst = screen[active];
  short n = PNUM;

  while (--n >= 0) {
    Area2D neon = *area++;

    if (neon.h > 0) {
      if (neon.h > 8) {
        neon.y += neon.h - 8;
        neon.h = 8; 
      }
      BlitterClearArea(dst, 4, &neon);
      BitmapCopyArea(dst, neon.x, neon.y, &background, &neon);
    }
  }
}

/* Copy visible slice of each logo; set mask planes for neon outline; scroll upward. */
static void DrawCliparts(void) {
  GreetingT *grt = greeting;
  Area2D *area = grt_area[active];
  BitmapT *dst = screen[active];
  short step = (frameCount - lastFrameCount) * 3;
  short n = PNUM;

  while (--n >= 0) {
    const BitmapT *src = grt->bitmap;
    short dy = grt->pos.y;
    short sy = 0;
    short sh = src->height;

    if (dy < 0) { sy -= dy; sh += dy; dy = 0; }
    if (dy + sh >= HEIGHT) { sh = HEIGHT - dy; }

    if (sh > 0) {
      Area2D bg_area = { grt->pos.x, dy, src->width, sh };
      Area2D fg_area = { 0, sy, src->width, sh };

      area->x = grt->pos.x;
      area->y = dy;
      area->w = src->width;
      area->h = sh;

      BitmapCopyArea(dst, grt->pos.x, dy, src, &fg_area);
      /*
       * Planes 0–2 hold the 4bpp greet art; planes 3–4 are a separable “glow” layer.
       * Two passes with different fill values / minterms paint outline variants
       * (grt->color 0 vs 1) so adjacent pens in the 32-colour palette read as neon.
       * Doing this in blitter passes is cheaper than a second full art pass on CPU.
       */
      BlitterSetArea(dst, 3, &bg_area, grt->color ? 0 : -1);
      BlitterSetArea(dst, 4, &bg_area, -1);
    } else {
      area->h = 0;
    }

    grt->pos.y -= step;
    grt++;
    area++;
  }
}

PROFILE(RenderNeons);

static void Render(void) {
  ProfilerStart(RenderNeons);
  {
    WaitBlitter();
    ClearCliparts();
    DrawCliparts();
  }
  ProfilerStop(RenderNeons);

  /*
   * Show the buffer we just drew; next frame uses the other bitmap so ClearCliparts
   * can read a stable grt_area from the previous field without tearing.
   */
  ITER(i, 0, DEPTH - 1, CopInsSet32(&bplptr[i], screen[active]->planes[i]));
  TaskWaitVBlank();
  active ^= 1;
}

EFFECT(Neons, Load, UnLoad, Init, Kill, Render, CustomRotatePalette);
