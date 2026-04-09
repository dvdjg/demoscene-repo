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
 * NewBitmap — allocate BitmapT and optionally CHIP (or PUBLIC) plane storage.
 *
 * bytesPerRow: width rounded up to 16 pixels, then /8 — one row of one plane in bytes.
 * bplSize: bytesPerRow * height — one full plane; total CHIP = bplSize * depth (roughly).
 * BM_MINIMAL: caller will set planes manually (InitSharedBitmap / static data).
 * BM_CPUONLY: MEMF_PUBLIC (fast) — not visible to DMA; for CPU-only scratch.
 * else: MEMF_CHIP — required for BPLxPT pointers and blitter SRC/DST in OCS.
 */
BitmapT *NewBitmap(u_short width, u_short height, u_short depth, u_char flags) {
  BitmapT *bitmap = MemAlloc(sizeof(BitmapT), MEMF_PUBLIC|MEMF_CLEAR);
  u_short bytesPerRow = ((width + 15) & ~15) / 8;

  bitmap->width = width;
  bitmap->height = height;
  bitmap->bytesPerRow = bytesPerRow;
  /* Let's make it aligned to short boundary. */
  bitmap->bplSize = bytesPerRow * height;
  bitmap->depth = depth;
  bitmap->flags = flags & BM_FLAGMASK;

  if (!(flags & BM_MINIMAL)) {
    u_int memoryFlags = 0;

    /* Recover memory flags. */
    if (flags & BM_CLEAR)
      memoryFlags |= MEMF_CLEAR;

    if (flags & BM_CPUONLY)
      memoryFlags |= MEMF_PUBLIC;
    else
      memoryFlags |= MEMF_CHIP;

    BitmapSetPointers(bitmap, MemAlloc(BitmapSize(bitmap), memoryFlags));
  }

  return bitmap;
}
