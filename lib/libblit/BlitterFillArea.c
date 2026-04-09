/*
 * Blitter-oriented code (libblit): DMA-friendly operations.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <blitter.h>

/*
 * BlitterFillArea — fill a rectangle (or whole plane) with a solid color using
 * blitter fill mode (SRCA + A_TO_D + FILL_OR + BLITREVERSE).
 *
 * area: if NULL, fills entire bitmap plane from end pointer backward (clears to 0
 * with fill carry). If set, computes bottom-right word address and BLTSIZE for
 * a subrect. BLITREVERSE: DMA walks backward so overlapping fills are safe.
 * C equivalent: memset on planar data is wrong — must set whole words per row;
 * blitter fill is the standard OCS way to clear large planes.
 */
void BlitterFillArea(const BitmapT *bitmap, short plane, const Area2D *area) {
  void *bltpt = bitmap->planes[plane];
  u_short bltmod, bltsize;

  if (area) {
    short x = area->x;
    short y = area->y; 
    short w = area->w;
    short h = area->h;

    bltpt += (((x + w) >> 3) & ~1) + (short)(y + h) * (short)bitmap->bytesPerRow;
    w >>= 3;
    bltmod = bitmap->bytesPerRow - w;
    bltsize = (h << 6) | (w >> 1);
  } else {
    bltpt += bitmap->bplSize;
    bltmod = 0;
    bltsize = (bitmap->height << 6) | (bitmap->bytesPerRow >> 1);
  }

  bltpt -= 2;

  WaitBlitter();

  custom->bltapt = bltpt;
  custom->bltdpt = bltpt;
  custom->bltamod = bltmod;
  custom->bltdmod = bltmod;
  custom->bltcon0 = (SRCA | DEST) | A_TO_D;
  custom->bltcon1 = BLITREVERSE | FILL_OR;
  custom->bltafwm = -1;
  custom->bltalwm = -1;
  custom->bltsize = bltsize;
}
