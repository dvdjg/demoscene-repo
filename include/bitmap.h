/*
 * Image/bitmap representation (bitmap.h) for CHIP-resident graphics.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
/*
 * bitmap.h — Estructura BitmapT para playfields de Amiga: ancho, alto, profundidad (planos),
 * bytesPerRow, flags (BM_CLEAR, BM_INTERLEAVED, etc.). Funciones para crear/liberar bitmaps
 * y prepararlos para DMA (BitmapMakeDisplayable). Los planos deben estar en chip RAM.
 */
#ifndef __BITMAP_H__
#define __BITMAP_H__

#include "common.h"

/* Extra sentinel space (one word) after plane data — blitter/copper safety margin. */
#define BM_EXTRA sizeof(u_short)

/*
 * BM_* flags — allocation and format of bitplanes.
 * BM_CLEAR: zero-initialize on alloc. BM_CPUONLY: not for DMA (debug / scratch).
 * BM_INTERLEAVED: planes interleaved in memory (AGA-style layout when used).
 * BM_MINIMAL: smaller allocator path. BM_HAM/BM_EHB: special display modes.
 * BM_STATIC: planes not heap-allocated (embedded arrays); DeleteBitmap must not free.
 */
#define BM_CLEAR        0x01
#define BM_CPUONLY      0x02
#define BM_INTERLEAVED  0x04
#define BM_MINIMAL      0x08
#define BM_HAM          0x10
#define BM_EHB          0x20
#define BM_STATIC       0x40 /* bitplanes were allocated statically */
#define BM_FLAGMASK     0x7F

#define BM_NPLANES 8

/*
 * BitmapT — OCS playfield: each plane is one bit per pixel per line; depth planes
 * combine to 2^depth colors (with palette). bytesPerRow ≥ ceil(width/16)*2 for lo-res.
 * planes[i] must be CHIP RAM for display DMA unless BM_CPUONLY.
 */
typedef struct Bitmap {
  u_short width;
  u_short height;
  u_short depth;
  u_short bytesPerRow;
  u_short bplSize;
  u_char flags;
  void *planes[BM_NPLANES];
} BitmapT;

u_int BitmapSize(BitmapT *bitmap);
void BitmapSetPointers(BitmapT *bitmap, void *planes);

void InitSharedBitmap(BitmapT *bitmap, u_short width, u_short height,
                      u_short depth, BitmapT *donor);

BitmapT *NewBitmap(u_short width, u_short height, u_short depth, u_char flags);
void DeleteBitmap(BitmapT *bitmap);
/* Pad rows, align buffers so BPLxPT/modulo work with hardware fetch. */
void BitmapMakeDisplayable(BitmapT *bitmap);

#endif
