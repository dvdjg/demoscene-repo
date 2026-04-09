/*
 * Blitter-oriented code (libblit): DMA-friendly operations.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <blitter.h>

void BitmapCopyArea(const BitmapT *dst, u_short x, u_short y,
                    const BitmapT *src, const Area2D *area)
{
  /* Only process planes that exist in both bitmaps.
   * Planar bit depth can differ between assets/effects. */
  short i, n = min(dst->depth, src->depth);

  /* Setup computes all geometry/masks once, then each Start() just binds
   * per-plane pointers and fires the DMA copy. This keeps the inner loop tiny. */
  BlitterCopyAreaSetup(dst, x, y, src, area);
  for (i = 0; i < n; i++)
    BlitterCopyAreaStart(i, i);
}
