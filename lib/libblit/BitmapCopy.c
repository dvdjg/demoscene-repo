/*
 * Blitter-oriented code (libblit): DMA-friendly operations.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <blitter.h>

/*
 * BitmapCopy — blit all overlapping planes from src to dst at (x,y).
 *
 * One BlitterCopySetup for geometry, then BlitterCopyStart per plane pair (i,i).
 * Depth limited by min(dst->depth, src->depth). C equivalent: nested loops with
 * per-pixel writes — impractical on OCS for large bitmaps.
 */
void BitmapCopy(const BitmapT *dst, u_short x, u_short y, const BitmapT *src) {
  /* Clamp to common depth so mismatched formats stay safe. */
  short i, n = min(dst->depth, src->depth);

  /* Setup once, then kick one blit per plane. */
  BlitterCopySetup(dst, x, y, src);
  for (i = 0; i < n; i++)
    BlitterCopyStart(i, i);
}
