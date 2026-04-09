/*
 * Blitter-oriented code (libblit): DMA-friendly operations.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <blitter.h>

void BitmapCopyMasked(const BitmapT *dst, u_short x, u_short y,
                      const BitmapT *src, const BitmapT *msk) 
{
  /* Plane count is clamped to prevent out-of-range plane pointers when source
   * and destination formats differ. */
  short i, n = min(dst->depth, src->depth);

  /* Mask is typically 1 bpp and reused for every destination plane.
   * This emulates classic sprite transparency using blitter minterms. */
  BlitterCopyMaskedSetup(dst, x, y, src, msk);
  for (i = 0; i < n; i++)
    BlitterCopyMaskedStart(i, i);
}
