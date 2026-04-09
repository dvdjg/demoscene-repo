/*
 * Glitch — RGB channel split + per-line horizontal shake (BPLCON1).
 *
 * Idea:
 * - The logo is copied into three bitplanes (DEPTH 3 → 8 colours). In normal
 *   mode the same artwork is stamped to each plane so the logo looks solid.
 * - “Glitch mode” (right mouse held): plane 0 and 1 are jittered by small random
 *   offsets while plane 2 stays fixed — the R/G/B planes no longer align, producing
 *   chromatic fringes. That mimics a broken cable/decoder without any true colour
 *   mode — still OCS planar.
 * - Each scanline’s `bplcon1` is patched to a random fine scroll 0..2 in both
 *   nibbles (PF1/PF2 shuffle). That shears the whole display horizontally in a
 *   noisy way, line by line — classic Copper “rolling horizontal offset” glitch.
 *
 * Pipeline:
 * - Blitter: `BitplaneCopyFast` stamps shifted copies (same as transparency effect).
 * - Copper: one WAIT + MOVE BPLCON1 per line; CPU patches `line[i]` each frame.
 * - `bplptr` refreshed so DMA reads the latest `screen[active]` bitmap after edits.
 *
 * `random()` is a tiny inline LCG in asm (`rol.l` + `addq`) for speed and
 * deterministic-ish chaos without pulling in full `libc` random.
 *
 * HRM (BPLCON1 scroll, Blitter): https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * HRM mirror: http://amigadev.elowar.com/read/
 */
#include "effect.h"
#include "copper.h"
#include "gfx.h"
#include "blitter.h"

#define WIDTH (160 + 32)
#define HEIGHT (128 + 32)
#define XSTART ((320 - WIDTH) / 2)
#define YSTART ((256 - HEIGHT) / 2)
#define DEPTH 3

static BitmapT *screen[2];
static short active = 0;
static CopInsPairT *bplptr;
static CopListT *cp;
/* Per-raster BPLCON1 data words (horizontal shuffle per line). */
static CopInsT *line[HEIGHT];

#include "data/ghostown-logo.c"

/* Same blitter recipe as other effects: SRCA→D + ASH shift for sub-word alignment. */
static void BitplaneCopyFast(BitmapT *dst, short d, u_short x, u_short y,
                             const BitmapT *src, short s)
{
  void *srcbpt = src->planes[s];
  void *dstbpt = dst->planes[d] + ((x & ~15) >> 3) + y * dst->bytesPerRow;
  u_short dstmod = dst->bytesPerRow - src->bytesPerRow;
  u_short bltsize = (src->height << 6) | (src->bytesPerRow >> 1);
  u_short bltshift = rorw(x & 15, 4);

  WaitBlitter();

  if (bltshift) {
    bltsize += 1; dstmod -= 2;

    custom->bltalwm = 0;
    custom->bltamod = -2;
  } else {
    custom->bltalwm = -1;
    custom->bltamod = 0;
  }

  custom->bltdmod = dstmod;
  custom->bltcon0 = (SRCA | DEST | A_TO_D) | bltshift;
  custom->bltcon1 = 0;
  custom->bltafwm = -1;
  custom->bltapt = srcbpt;
  custom->bltdpt = dstbpt;
  custom->bltsize = bltsize;
}

static CopListT *MakeCopperList(void) {
  CopListT *cp = NewCopList(100 + HEIGHT * 2);
  short i;

  bplptr = CopSetupBitplanes(cp, screen[active], DEPTH);
  for (i = 0; i < HEIGHT; i++) {
    CopWait(cp, Y(YSTART + i), HP(0));
    line[i] = CopMove16(cp, bplcon1, 0);
  }
  return CopListFinish(cp);
}

/* Centred window, 3-plane lores, 8 colour entries for logo + fringes. */
static void Init(void) {
  screen[0] = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR);
  screen[1] = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR);

  SetupPlayfield(MODE_LORES, DEPTH, X(XSTART), Y(YSTART), WIDTH, HEIGHT);
  SetColor(0, 0x000);
  SetColor(1, 0x00F);
  SetColor(2, 0x0F0);
  SetColor(3, 0xF00);
  SetColor(4, 0x0FF);
  SetColor(5, 0xF0F);
  SetColor(6, 0xFF0);
  SetColor(7, 0xFFF);

  cp = MakeCopperList();
  CopListActivate(cp);
  EnableDMA(DMAF_RASTER | DMAF_BLITTER);
}

static void Kill(void) {
  /* Also stop copper DMA so lists are not fetched while freeing. */
  DisableDMA(DMAF_COPPER | DMAF_RASTER | DMAF_BLITTER);

  DeleteCopList(cp);
  DeleteBitmap(screen[0]);
  DeleteBitmap(screen[1]);
}

/*
 * Fast pseudo-random: rotate 32-bit seed left, add small constant.
 * Conceptual C: `seed = (seed << 1) | (seed >> 31); seed += 5; return (u_short)seed;`
 * Why asm: minimal instruction count in the inner Render path; not cryptographic.
 */
static inline u_short random(void) {
  static u_int seed = 0xDEADC0DE;

  asm("rol.l  %0,%0\n"
      "addq.l #5,%0\n"
      : "+d" (seed));

  return seed;
}

PROFILE(RenderGlitch);

static void Render(void) {
  short i;

  ProfilerStart(RenderGlitch);

  if (RightMouseButton()) {
    /*
     * Glitch: decouple planes 0/1 from plane 2 with small random shifts; zero
     * BPLCON1 so fine scroll does not fight the colour split.
     */
    int x1 = (random() % 5) - 2;
    int x2 = (random() % 5) - 2;
    int y1 = (random() % 5) - 2;
    int y2 = (random() % 5) - 2;

    BitplaneCopyFast(screen[active], 0, 16 + x1, 16 + y1, &logo, 0);
    BitplaneCopyFast(screen[active], 1, 16 + x2, 16 + y2, &logo, 0);
    BitplaneCopyFast(screen[active], 2, 16, 16, &logo, 0);

    for (i = 0; i < HEIGHT; i++)
      CopInsSet16(line[i], 0);
  } else {
    /* Normal: aligned logo on all planes; per-line random 0..2 pixel shuffle. */
    BitplaneCopyFast(screen[active], 0, 16, 16, &logo, 0);
    BitplaneCopyFast(screen[active], 1, 16, 16, &logo, 0);
    BitplaneCopyFast(screen[active], 2, 16, 16, &logo, 0);

    for (i = 0; i < HEIGHT; i++) {
      short shift = random() % 3;
      CopInsSet16(line[i], (shift << 4) | shift);
    }
  }

  ProfilerStop(RenderGlitch);

  /* Point DMA at the bitmap we just drew (same buffer index as blitter target). */
  ITER(i, 0, DEPTH - 1, CopInsSet32(&bplptr[i], screen[active]->planes[i]));
  TaskWaitVBlank();
  active ^= 1;
}

EFFECT(Glitch, NULL, NULL, Init, Kill, Render, NULL);
