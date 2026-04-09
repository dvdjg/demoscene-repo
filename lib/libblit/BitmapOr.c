/*
 * Blitter-oriented code (libblit): DMA-friendly operations.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <blitter.h>

/*
 * BitmapOr — OR src into dst at (x,y) for each overlapping plane (blitter A_OR_B).
 *
 * SRCB reads existing dst (read-modify-write); SRCA is source. Used for additive
 * stamping without erasing destination bits. Same setup pattern as BitmapCopy with
 * different minterm.
 */
void BitmapOr(const BitmapT *dst, u_short x, u_short y, const BitmapT *src) {
  /* OR is applied only on overlapping planes. */
  short i, n = min(dst->depth, src->depth);

  /* Setup shared geometry/registers once; Start performs per-plane RMW. */
  BlitterOrSetup(dst, x, y, src);
  for (i = 0; i < n; i++)
    BlitterOrStart(i, i);
}
