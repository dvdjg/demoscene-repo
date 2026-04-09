/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <bitmap.h>

/*
 * BitmapSetPointers — lay out plane base addresses from one contiguous `planes` block.
 *
 * Planar (default): stride between planes = bplSize (one full plane after another).
 * BM_INTERLEAVED: stride = bytesPerRow (AGA-style interleaved lines — rare in OCS demos).
 * Loop runs depth+1 times (`do while (depth--)`): if that does not match your depth
 * convention, verify against callers.
 * NOTE: possible issue: off-by-one vs `depth` plane count — compare with BM_NPLANES use.
 */
void BitmapSetPointers(BitmapT *bitmap, void *planes) {
  int modulo =
    (bitmap->flags & BM_INTERLEAVED) ? bitmap->bytesPerRow : bitmap->bplSize;
  short depth = bitmap->depth;
  void **planePtr = bitmap->planes;

  do {
    *planePtr++ = planes;
    planes += modulo;
  } while (depth--);
}
