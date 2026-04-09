/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <bitmap.h>
#include <string.h>
#include <system/memory.h>

/*
 * BitmapMakeDisplayable — copy FAST/PUBLIC CPU buffer into CHIP and clear BM_CPUONLY.
 *
 * DMA bitplanes must live in CHIP on classic Amiga; this migrates pixels after
 * software rendering. memcpy size is BitmapSize - BM_EXTRA (padding preserved).
 */
void BitmapMakeDisplayable(BitmapT *bitmap) {
  if (!(bitmap->flags & BM_CPUONLY)) {
    u_int size = BitmapSize(bitmap);
    void *planes = MemAlloc(size, MEMF_CHIP);

    memcpy(planes, bitmap->planes[0], size - BM_EXTRA);
    MemFree(bitmap->planes[0]);

    bitmap->flags ^= BM_CPUONLY;
    BitmapSetPointers(bitmap, planes);
  }
}
