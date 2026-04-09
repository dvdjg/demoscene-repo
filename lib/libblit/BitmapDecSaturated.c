/*
 * Blitter-oriented code (libblit): DMA-friendly operations.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <blitter.h>

/*
 * BitmapDecSaturated — decrement dst with floor at zero (saturating subtract 1).
 *
 * borrow_bm supplies two temporary borrow planes (borrow0/borrow1 ping-pong).
 * Plane 0 starts with HALF_SUB; higher planes propagate borrow with HALF_SUB_BORROW.
 * Final pass masks destination with NOT borrow (A_AND_NOT_B) to clamp underflow.
 */
void BitmapDecSaturated(const BitmapT *dst_bm, const BitmapT *borrow_bm) {
  void *borrow0 = borrow_bm->planes[0];
  void *borrow1 = borrow_bm->planes[1];
  void *const *dst = dst_bm->planes;
  void *ptr = *dst++;
  u_short bltsize = (dst_bm->height << 6) | (dst_bm->bytesPerRow >> 1);
  short n = dst_bm->depth - 1;

  /* Register baseline for subtraction passes. */
  WaitBlitter();
  custom->bltcon1 = 0;
  custom->bltamod = 0;
  custom->bltbdat = -1;
  custom->bltbmod = 0;
  custom->bltdmod = 0;
  custom->bltafwm = -1;
  custom->bltalwm = -1;

  custom->bltapt = ptr;
  custom->bltdpt = borrow0;
  custom->bltcon0 = HALF_SUB_BORROW & ~SRCB;
  custom->bltsize = bltsize;

  WaitBlitter();
  custom->bltapt = ptr;
  custom->bltdpt = ptr;
  custom->bltcon0 = HALF_SUB & ~SRCB;
  custom->bltsize = bltsize;

  /* Borrow propagation across planes (least -> most significant). */
  while (--n >= 0) {
    ptr = *dst++;

    WaitBlitter();
    custom->bltapt = ptr;
    custom->bltbpt = borrow0;
    custom->bltdpt = borrow1;
    custom->bltcon0 = HALF_SUB_BORROW;
    custom->bltsize = bltsize;

    WaitBlitter();
    custom->bltapt = ptr;
    custom->bltbpt = borrow0;
    custom->bltdpt = ptr;
    custom->bltcon0 = HALF_SUB;
    custom->bltsize = bltsize;

    swapr(borrow0, borrow1);
  }

  /* Clamp pass: clear bits where final borrow indicates underflow. */
  dst = dst_bm->planes;
  n = dst_bm->depth;
  while (--n >= 0) {
    ptr = *dst++;

    WaitBlitter();
    custom->bltapt = ptr;
    custom->bltbpt = borrow0;
    custom->bltdpt = ptr;
    custom->bltcon0 = (SRCA | SRCB | DEST) | A_AND_NOT_B;
    custom->bltsize = bltsize;
  }
}
