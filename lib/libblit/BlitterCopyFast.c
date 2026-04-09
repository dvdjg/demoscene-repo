/*
 * Blitter-oriented code (libblit): DMA-friendly operations.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <blitter.h>

/*
 * Deferred state for BlitterCopyFastSetup / Start — same pattern as BlitterCopy.c
 * but assumes src width fits dst stride (no extra width calc); optimized path.
 */
typedef struct {
  const BitmapT *src;
  const BitmapT *dst;
  u_int start;
  u_short size;
} StateT;

static StateT state[1];

void BlitterCopyFastSetup(const BitmapT *dst, u_short x, u_short y,
                          const BitmapT *src) 
{
  /* Fast path assumes full source row width; only destination modulo is needed. */
  u_short dstmod = dst->bytesPerRow - src->bytesPerRow;
  u_short bltshift = rorw(x & 15, 4);
  u_short bltsize = (src->height << 6) | (src->bytesPerRow >> 1);

  if (bltshift)
    bltsize++, dstmod -= 2;

  state->src = src;
  state->dst = dst;
  state->start = ((x & ~15) >> 3) + y * dst->bytesPerRow;
  state->size = bltsize;

  WaitBlitter();

  if (bltshift) {
    /* Shifted copies consume an extra trailing word. */
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
}

void BlitterCopyFastStart(short dstbpl, short srcbpl) {
  /* Per-plane kick; geometry/state already prepared by Setup. */
  void *srcbpt = state->src->planes[srcbpl];
  void *dstbpt = state->dst->planes[dstbpl] + state->start;
  u_short bltsize = state->size;

  WaitBlitter();

  custom->bltapt = srcbpt;
  custom->bltdpt = dstbpt;
  custom->bltsize = bltsize;
}
