/*
 * Blitter-oriented code (libblit): DMA-friendly operations.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <blitter.h>

BitmapT *BitmapMakeMask(const BitmapT *bitmap) {
  /* Result is 1bpp: any non-zero source bit in any plane becomes opaque mask.
   * This is a common pre-pass before masked blits on planar sprites. */
  BitmapT *mask = NewBitmap(bitmap->width, bitmap->height, 1, 0);
  u_short bltsize = (bitmap->height << 6) | (bitmap->bytesPerRow >> 1);
  void *const *planes = bitmap->planes;
  void *dst = mask->planes[0];
  short n = bitmap->depth;

  while (--n >= 0) {
    void *src = *planes++;

    WaitBlitter();

    custom->bltamod = 0;
    custom->bltbmod = 0;
    custom->bltdmod = 0;
    custom->bltcon0 = (SRCA | SRCB | DEST) | A_OR_B;
    custom->bltcon1 = 0;
    custom->bltafwm = -1;
    custom->bltalwm = -1;

    custom->bltapt = src;
    custom->bltbpt = dst;
    custom->bltdpt = dst;
    /* D = A OR B accumulates all source planes into one mask plane. */
    custom->bltsize = bltsize;
  }

  return mask;
}
