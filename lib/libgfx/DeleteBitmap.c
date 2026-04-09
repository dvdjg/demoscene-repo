/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <bitmap.h>
#include <system/memory.h>

/*
 * DeleteBitmap — free bitmap struct and contiguous plane block (if allocated).
 *
 * planes[0] holds the base of all planes in one MemAlloc chunk (see BitmapSetPointers).
 * BM_MINIMAL: user owns storage — only free the BitmapT struct.
 */
void DeleteBitmap(BitmapT *bitmap) {
  if (bitmap) {
    if (!(bitmap->flags & BM_MINIMAL))
      MemFree(bitmap->planes[0]);
    MemFree(bitmap);
  }
}
