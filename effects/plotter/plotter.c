/*
 * Plotter — spirograph-style additive flares + copper-driven sprite columns + gradient sky.
 *
 * `DrawPlotter` places NUM stamps per frame along a polar rose (`SIN`/`COS` with `ARMS`
 * frequency); `BitmapAddSaturated` merges 16×16 flare tiles into the playfield using
 * the ripple-carry style blitter add (see lib `BitmapAddSaturated`). `carry` is a
 * scratch plane buffer for the multi-pass add.
 *
 * The copper list is large: every scanline it (1) moves eight sprites’ horizontal
 * positions for a left block, (2) loads three RGB triplets into several colour
 * registers from a vertical gradient stream (`background_cols_pixels`), (3) repeats
 * sprite positions for a right block after CopWait at x=128 — so the same 8
 * sprites draw two vertical “curtain” bands with per-line hue shifts.
 *
 * `Render` swaps background sprite sets (`background_1/2/3`) every few frames for
 * animation; `bplptr` tracks the double-buffered bitmap.
 *
 * Why only eight sprites for two columns: OCS allows eight DMA sprites. The same
 * eight are drawn twice per scanline — WAIT at x=128 moves their HPOS — so you get
 * two vertical “curtains” without exceeding hardware limits (time-slicing the same
 * sprite objects at two horizontal positions).
 *
 * Why the same RGB triplet is written to four groups of colour registers: sprite
 * colour attachment uses pens 16–31 (four 3-colour blocks for pairs 0–7). Duplicating
 * the gradient keeps all eight sprites visually matched to the per-line sky gradient.
 *
 * HRM: https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * HRM mirror: http://amigadev.elowar.com/read/
 */
#include <effect.h>
#include <blitter.h>
#include <circle.h>
#include <copper.h>
#include <fx.h>
#include <gfx.h>
#include <sprite.h>
#include <system/memory.h>

#define WIDTH 256
#define HEIGHT 256
#define DEPTH 3
#define SIZE 16
#define NUM 37
#define ARMS 3
/* Amplitude clamp for rose radius (see DrawPlotter). */
#define MAX (96+16)

static __code CopListT *cp;
static __code CopInsPairT *bplptr;
static __code CopInsPairT *sprptr;
static __code BitmapT *screen[2];
static __code short active = 0;
/* BitmapAddSaturated scratch (ripple-carry across two planes). */
static __code BitmapT *carry;
/* Pre-cut 16×16 flare tiles from atlas flares (intensity index 0..7). */
static __code BitmapT *flare[8];

#include "data/plotter-flares.c"
#include "data/background-1.c"
#include "data/background-2.c"
#include "data/background-3.c"
#include "data/gradient.c"

/*
 * Per-line: position sprites for the left column, load gradient pens, WAIT mid-line,
 * reposition the same sprites for the right column — all before the beam passes.
 */
static CopListT *MakeCopperList(void) {
  CopListT *cp = NewCopList(50 + HEIGHT * (9 + 9 + 13));
  u_short *cols = background_cols_pixels;
  short i, j;

  sprptr = CopSetupSprites(cp);
  for (i = 0; i < 8; i++) {
    CopInsSetSprite(&sprptr[i], background_1[i]);
    SpriteUpdatePos(background_1[i], X(16 * i), Y(0));
  }

  CopWait(cp, Y(-1), HP(0));
  bplptr = CopSetupBitplanes(cp, screen[active], DEPTH);

  for (i = 0; i < HEIGHT; i++) {
    u_short c0, c1, c2;

    CopWaitSafe(cp, Y(i), HP(0));

    for (j = 0; j < 8; j++)
      CopMove16(cp, spr[j].pos, SPRPOS(DIWHP + 16*j + 32, DIWVP + i));

    /* One word stride per row in background_cols, then three RGB12 words. */
    cols++;
    c0 = *cols++;
    c1 = *cols++;
    c2 = *cols++;

    CopMove16(cp, color[17], c0);
    CopMove16(cp, color[18], c1);
    CopMove16(cp, color[19], c2);
    CopMove16(cp, color[21], c0);
    CopMove16(cp, color[22], c1);
    CopMove16(cp, color[23], c2);
    CopMove16(cp, color[25], c0);
    CopMove16(cp, color[26], c1);
    CopMove16(cp, color[27], c2);
    CopMove16(cp, color[29], c0);
    CopMove16(cp, color[30], c1);
    CopMove16(cp, color[31], c2);

    CopWaitSafe(cp, Y(i), X(128));
    for (j = 0; j < 8; j++)
      CopMove16(cp, spr[j].pos, SPRPOS(DIWHP + 16*j + 128 + 32, DIWVP + i));
  }

  CopListFinish(cp);
  return cp;
}

static void Init(void) {
  short i;

  screen[0] = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR);
  screen[1] = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR);

  EnableDMA(DMAF_BLITTER);

  for (i = 0; i < 8; i++) {
    Area2D flare_area = { 0, i * SIZE, SIZE, SIZE };
    flare[i] = NewBitmap(SIZE, SIZE, DEPTH, 0);
    BitmapClear(flare[i]);
    BitmapCopyArea(flare[i], 0, 0, &flares, &flare_area);
  }

  carry = NewBitmap(SIZE + 16, SIZE, 2, 0);

  for (i = 0; i < 2; i++)
    BitmapClear(screen[i]);

  SetupPlayfield(MODE_LORES, DEPTH, X((320 - WIDTH) / 2), Y(0), WIDTH, HEIGHT);
  LoadColors(flares_colors, 0);
  LoadColors(background_colors, 16);
  LoadColors(background_colors, 20);
  LoadColors(background_colors, 24);
  LoadColors(background_colors, 28);

  /* Sprites in front of bitplanes (priority bits in bplcon2). */
  custom->bplcon2 = 0;

  cp = MakeCopperList();
  CopListActivate(cp);
  EnableDMA(DMAF_RASTER | DMAF_SPRITE | DMAF_BLITHOG);
}

static void Kill(void) {
  DisableDMA(DMAF_COPPER | DMAF_BLITTER | DMAF_RASTER | DMAF_SPRITE | DMAF_BLITHOG);

  ITER(i, 0, 7, DeleteBitmap(flare[i]));
  DeleteBitmap(carry);
  DeleteBitmap(screen[0]);
  DeleteBitmap(screen[1]);
  DeleteCopList(cp);
}

/* Lay down NUM flare sprites along a Lissajous/spiro path; intensity index from |g|. */
static void DrawPlotter(BitmapT *screen, short frameCount) {
  short i, a;
  short t = frameCount * 5;
  short da = 2 * SIN_PI / NUM;

  for (i = 0, a = t; i < NUM; i++, a += da) {
    short g = SIN(ARMS * a);
    short x = normfx(normfx(SIN(t + a) * g) * MAX) + 128 - SIZE / 2;
    short y = normfx(normfx(COS(t + a) * g) * MAX) + 128 - SIZE / 2;
    short f = normfx(g * 3);

    if (f < 0)
      f = -f;

    /* Occasional full-bright stamp on odd indices — breaks uniformity of the rose. */
    if ((i & 1) && (frameCount & 15) < 3)
      f = 7;

    BitmapAddSaturated(screen, x, y, flare[f], carry);
  }
}

static __code SpriteT **background[3] = {
  background_1,
  background_2,
  background_3,
};

PROFILE(Draw);

static void Render(void) {
  ProfilerStart(Draw);
  {
    BitmapClear(screen[active]);
    DrawPlotter(screen[active], frameCount);
  }
  ProfilerStop(Draw);

  ITER(i, 0, DEPTH - 1, CopInsSet32(&bplptr[i], screen[active]->planes[i]));
  TaskWaitVBlank();
  active ^= 1;

  /*
   * Sprite definitions are switched after the field so the copper list still
   * references valid sprpt data for the frame we just displayed; next field uses
   * the new background_* art.
   */
  {
    short i;
    short num = mod16(div16(frameCount, 5), 3);

    for (i = 0; i < 8; i++) {
      CopInsSetSprite(&sprptr[i], background[num][i]);
      SpriteUpdatePos(background[num][i], X(16 * i), Y(0));
    }
  }
}

EFFECT(Plotter, NULL, NULL, Init, Kill, Render, NULL);
