/*
 * MetaBalls — additive composition of three circular kernels into 5 bitplanes.
 *
 * Each metaball is the same pre-rendered radial falloff bitmap (`metaball`). The
 * first is copied on; the next two are merged with `BitmapAddSaturated` so plane
 * values sum in CHIP (ripple-carry style add — see lib blit). Where the field crosses
 * palette thresholds, the 5-plane display reads as iso-surfaces of “blob” fusion.
 *
 * Side columns (`bgLeft` / `bgRight`) are static art; the central strip is cleared
 * each frame then redrawn. `carry` is the 2-plane scratch for saturated adds.
 *
 * HRM: https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * HRM mirror: http://amigadev.elowar.com/read/
 */

#include <effect.h>
#include <2d.h>
#include <blitter.h>
#include <copper.h>
#include <fx.h>
#include <system/memory.h>

#define WIDTH 320
#define HEIGHT 256
#define DEPTH 5
#define SIZE 80

static BitmapT *screen[2];
static short active = 0;

/* Double-buffered ball positions [buffer][ball] — only `active` row is written. */
static Point2D pos[2][3];
static BitmapT *carry;
static CopInsPairT *bplptr;
static CopListT *cp;

#include "data/metaball.c"
#include "data/metaball-bg-left-1.c"
#include "data/metaball-bg-right-1.c"

/* Placeholder before first PositionMetaballs (see also frame 0 animation). */
static void SetInitialPositions(void) {
  short i, j;

  for (i = 0; i < 2; i++) {
    for (j = 0; j < 3; j++) {
      pos[i][j].x = 160;
      pos[i][j].x = 128;
    }
  }
}

static void Init(void) {
  short j;

  screen[0] = NewBitmap(WIDTH, HEIGHT, DEPTH, 0);
  screen[1] = NewBitmap(WIDTH, HEIGHT, DEPTH, 0);

  EnableDMA(DMAF_BLITTER | DMAF_BLITHOG);

  for (j = 0; j < 2; j++) {
    BitmapClearArea(screen[j], &((Area2D){32, 0, WIDTH - 64, HEIGHT}));
    BitmapCopy(screen[j], 0, 0, &bgLeft);
    BitmapCopy(screen[j], WIDTH - 32, 0, &bgRight);
  }

  carry = NewBitmap(SIZE + 16, SIZE, 2, 0);

  SetInitialPositions();

  SetupPlayfield(MODE_LORES, DEPTH, X(0), Y(0), WIDTH, HEIGHT);
  LoadColors(metaball_colors, 0);

  cp = NewCopList(100);
  bplptr = CopSetupBitplanes(cp, screen[active], DEPTH);
  CopListFinish(cp);
  CopListActivate(cp);
  EnableDMA(DMAF_RASTER);
}

static void Kill(void) {
  DisableDMA(DMAF_COPPER | DMAF_RASTER | DMAF_BLITTER | DMAF_BLITHOG);

  DeleteBitmap(carry);
  DeleteCopList(cp);
  DeleteBitmap(screen[0]);
  DeleteBitmap(screen[1]);
}

/* Erase previous frame’s three SIZE×SIZE stamps (x aligned to 16 for clear area). */
static void ClearMetaballs(void) {
  Area2D mball = {0, 0, SIZE + 16, SIZE};
  short *val = (short *)pos[active];
  short n = 3;

  while (--n >= 0) {
    mball.x = *val++ & ~15;
    mball.y = *val++;
    BitmapClearArea(screen[active], &mball);
  }
}

/* Lissajous-style motion for the three balls in the central band. */
static void PositionMetaballs(void) {
  int t = frameCount * 24;
  short *val = (short *)pos[active];


  *val++ = (WIDTH - SIZE) / 2 + normfx(SIN(t) * SIZE * 3 / 4);
  *val++ = (HEIGHT - SIZE) / 2;
  *val++ = (WIDTH - SIZE) / 2 - normfx(SIN(t) * SIZE * 3 / 4);
  *val++ = (HEIGHT - SIZE) / 2;
  *val++ = (WIDTH - SIZE) / 2;
  *val++ = (HEIGHT - SIZE) / 2 + normfx(COS(t) * SIZE * 3 / 4);
}

/* First ball: raw copy; others: add into existing field (metabolic overlap). */
static void DrawMetaballs(void) {
  short *val = (short *)pos[active];
  int x, y;

  x = *val++; y = *val++; BitmapCopyFast(screen[active], x, y, &metaball);
  x = *val++; y = *val++; BitmapAddSaturated(screen[active], x, y, &metaball, carry);
  x = *val++; y = *val++; BitmapAddSaturated(screen[active], x, y, &metaball, carry);
}

PROFILE(Metaballs);

static void Render(void) {
  ProfilerStart(Metaballs);
  {
    ClearMetaballs();
    PositionMetaballs();
    DrawMetaballs();
  }
  ProfilerStop(Metaballs);

  ITER(i, 0, DEPTH - 1, CopInsSet32(&bplptr[i], screen[active]->planes[i]));
  TaskWaitVBlank();
  active ^= 1;
}

EFFECT(MetaBalls, NULL, NULL, Init, Kill, Render, NULL);
