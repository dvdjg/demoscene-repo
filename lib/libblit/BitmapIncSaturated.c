/*
 * Blitter-oriented code (libblit): DMA-friendly operations.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <blitter.h>

/*
 * BitmapIncSaturated — increment selected pixels in dst (with clamp at max colour).
 *
 * Selection mask is carry_bm->planes[0]: bit=1 means "increment this pixel".
 * Algorithm is identical to multi-bit integer +1 propagation:
 * - generate carry per plane (HALF_ADDER_CARRY),
 * - write plane sum (HALF_ADDER),
 * - propagate carry through all planes,
 * - OR final carry back (saturation).
 */
void BitmapIncSaturated(const BitmapT *dst_bm, const BitmapT *carry_bm) {
  void *carry0 = carry_bm->planes[0];
  void *carry1 = carry_bm->planes[1];
  void *const *dst = dst_bm->planes;
  void *ptr;
  u_short bltsize = (dst_bm->height << 6) | (dst_bm->bytesPerRow >> 1);
  short n = dst_bm->depth;

  /* Only pixels set in carry0 are increment candidates. */

  WaitBlitter();
  custom->bltcon1 = 0;
  custom->bltamod = 0;
  custom->bltbmod = 0;
  custom->bltdmod = 0;
  custom->bltafwm = -1;
  custom->bltalwm = -1;

  /* Ripple-carry increment through all bitplanes. */
  while (--n >= 0) {
    ptr = *dst++;

    WaitBlitter();
    custom->bltapt = ptr;
    custom->bltbpt = carry0;
    custom->bltdpt = carry1;
    custom->bltcon0 = HALF_ADDER_CARRY;
    custom->bltsize = bltsize;

    WaitBlitter();
    custom->bltapt = ptr;
    custom->bltbpt = carry0;
    custom->bltdpt = ptr;
    custom->bltcon0 = HALF_ADDER;
    custom->bltsize = bltsize;

    swapr(carry0, carry1);
  }

  /* Clamp pass: if carry still set after top plane, force destination bits on. */
  dst = dst_bm->planes;
  n = dst_bm->depth;
  while (--n >= 0) {
    ptr = *dst++;

    WaitBlitter();
    custom->bltapt = ptr;
    custom->bltbpt = carry0;
    custom->bltdpt = ptr;
    custom->bltcon0 = (SRCA | SRCB | DEST) | A_OR_B;
    custom->bltsize = bltsize;
  }
}
