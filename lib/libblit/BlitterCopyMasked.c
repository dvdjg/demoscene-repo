/*
 * Blitter-oriented code (libblit): DMA-friendly operations.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <blitter.h>

typedef struct {
  /* Cached pointers and geometry shared between Setup/Start calls. */
  const BitmapT *src;
  const BitmapT *msk;
  const BitmapT *dst;
  u_int start;
  u_short size;
} StateT;

static StateT state[1];

/*
 * BlitterCopyMaskedSetup — configure minterms/modulo for masked copy (full src rect).
 *
 * Channel roles (HRM Blitter chapter): A = source plane, B = mask plane (1 bpp),
 * C = destination read for merge, D = write. Minterm (ABC|ABNC|ANBC|NANBC) is the
 * classic "replace dest with src where mask=1" pattern.
 *
 * dst: destination bitmap; x,y: top-left in destination where the src bitmap is placed.
 * src: full rectangular bitmap; msk: single-plane mask (same geometry as src).
 */
void BlitterCopyMaskedSetup(const BitmapT *dst, u_short x, u_short y,
                            const BitmapT *src, const BitmapT *msk)
{
  u_short dstmod = dst->bytesPerRow - src->bytesPerRow;
  u_short bltsize = (src->height << 6) | (src->bytesPerRow >> 1);
  u_short bltshift = rorw(x & 15, 4);

  state->src = src;
  state->dst = dst;
  state->msk = msk;
  state->start = ((x & ~15) >> 3) + y * dst->bytesPerRow;

  if (bltshift)
    bltsize++, dstmod -= 2;

  /* Final BLTSIZE for BlitterCopyMaskedStart (must match registers below). */
  state->size = bltsize;

  WaitBlitter();

  if (bltshift) {
    custom->bltamod = -2;
    custom->bltbmod = -2;
    custom->bltcon0 = (SRCA | SRCB | SRCC | DEST) | (ABC | ABNC | ANBC | NANBC) | bltshift;
    custom->bltcon1 = bltshift;
    custom->bltalwm = 0;
    custom->bltafwm = -1;
    custom->bltcmod = dstmod;
    custom->bltdmod = dstmod;
  } else {
    custom->bltamod = 0;
    custom->bltbmod = 0;
    custom->bltcon0 = (SRCA | SRCB | SRCC | DEST) | (ABC | ABNC | ANBC | NANBC);
    custom->bltcon1 = 0;
    custom->bltalwm = -1;
    custom->bltafwm = -1;
    custom->bltcmod = dstmod;
    custom->bltdmod = dstmod;
  }
}

/*
 * BlitterCopyMaskedStart — bind plane pointers and kick one masked blit.
 *
 * dstbpl/srcbpl: which bitplane index to use on dst/src (same index is typical).
 * Waits for idle blitter before programming pointers (serializes with prior blits).
 */
void BlitterCopyMaskedStart(short dstbpl, short srcbpl) {
  void *srcbpt = state->src->planes[srcbpl];
  void *dstbpt = state->dst->planes[dstbpl] + state->start;
  void *mskbpt = state->msk->planes[0];
  u_short bltsize = state->size;

  WaitBlitter();

  custom->bltapt = srcbpt;
  custom->bltbpt = mskbpt;
  custom->bltcpt = dstbpt;
  custom->bltdpt = dstbpt;
  custom->bltsize = bltsize;
}
