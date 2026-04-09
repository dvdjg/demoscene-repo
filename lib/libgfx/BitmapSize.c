/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <bitmap.h>

/*
 * BitmapSize — total bytes to allocate for all bitplanes plus BM_EXTRA.
 *
 * BM_EXTRA: padding after last plane (blitter line mode may read past end);
 * keeps DMA from walking into unrelated CHIP.
 */
u_int BitmapSize(BitmapT *bitmap) {
  /* Allocate extra two bytes for scratchpad area.
   * Used by blitter line drawing. */
  return ((u_short)bitmap->bplSize * (u_short)bitmap->depth) + BM_EXTRA;
}
