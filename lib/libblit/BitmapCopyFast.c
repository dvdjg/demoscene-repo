/*
 * Blitter-oriented code (libblit): DMA-friendly operations.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <blitter.h>

void BitmapCopyFast(const BitmapT *dst, u_short x, u_short y,
                    const BitmapT *src) 
{
  /* Copy only common planes; extra destination planes are intentionally left
   * untouched so callers can layer graphics by plane. */
  short i, n = min(dst->depth, src->depth);

  /* "Fast" path skips area clipping math done by BitmapCopyArea and assumes
   * full-source copy semantics, which is common in sprite/effect blits. */
  BlitterCopyFastSetup(dst, x, y, src);
  for (i = 0; i < n; i++)
    BlitterCopyFastStart(i, i);
}
