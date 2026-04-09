/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <pixmap.h>
#include <system/memory.h>

/* bitdepth — per PM_DEPTH_* entry: bits per pixel for indexed modes. */
static short bitdepth[] = { 0, 4, 8 };

/*
 * PixmapSize — bytes needed for chunky buffer (RGB triple layout or indexed bytes).
 */
static int PixmapSize(PixmapT *pixmap) {
  short bitsPerPixel = bitdepth[pixmap->type & PM_DEPTH_MASK];
  short bytesPerRow;
  if (pixmap->type & _PM_RGB) {
    bitsPerPixel = (bitsPerPixel * 3 + 7) >> 3;
    bytesPerRow = bitsPerPixel * pixmap->width;
  } else {
    bytesPerRow = (bitsPerPixel * pixmap->width + 7) >> 3;
  }
  return bytesPerRow * pixmap->height;
}

/*
 * NewPixmap — allocate PixmapT + pixel buffer (MEMF per caller flags: CHIP or FAST).
 */
PixmapT *NewPixmap(short width, short height, PixmapTypeT type,
                   u_int memoryAttributes)
{
  PixmapT *pixmap = MemAlloc(sizeof(PixmapT), MEMF_PUBLIC|MEMF_CLEAR);

  pixmap->type = type;
  pixmap->width = width;
  pixmap->height = height;
  pixmap->pixels = MemAlloc(PixmapSize(pixmap), memoryAttributes);

  return pixmap;
}
