/*
 * Image/bitmap representation (pixmap.h) for CHIP-resident graphics.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#ifndef __PIXMAP_H__
#define __PIXMAP_H__

#include "gfx.h"

/*
 * Pixmap — chunky pixels (indexed or RGB nibbles) vs planar BitmapT.
 * Used for tools / c2p path: CPU fills chunky buffer, then scrambles to bitplanes.
 */
typedef enum {
  PM_NONE, PM_DEPTH_4, PM_DEPTH_8,
  _PM_CMAP = 4,
  _PM_RGB  = 8,
} PixmapTypeT;

#define PM_CMAP4  (_PM_CMAP|PM_DEPTH_4)
#define PM_CMAP8  (_PM_CMAP|PM_DEPTH_8)
#define PM_RGB12  (_PM_RGB|PM_DEPTH_4)

#define PM_DEPTH_MASK 3

typedef struct Pixmap {
  PixmapTypeT type;
  short width, height;
  void *pixels;
} PixmapT;

static inline void InitSharedPixmap(PixmapT *pixmap, short width, short height,
                                    PixmapTypeT type, void *pixels) 
{
  pixmap->type = type;
  pixmap->width = width;
  pixmap->height = height;
  pixmap->pixels = pixels;
}

PixmapT *NewPixmap(short width, short height, 
                             PixmapTypeT type, u_int memFlags);
void DeletePixmap(PixmapT *pixmap);

/*
 * PixmapScramble_* — reorder bytes for c2p (chunky-to-planar) blitter layouts.
 * _4_1 / _4_2 differ in interleaving pattern for 4 bitplanes; see lib implementation.
 */
void PixmapScramble_4_1(const PixmapT *pixmap);
void PixmapScramble_4_2(const PixmapT *pixmap);

#endif
