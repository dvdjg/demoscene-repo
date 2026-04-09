/*
 * Blitter-oriented code (libblit): DMA-friendly operations.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <blitter.h>

void BitmapSetArea(const BitmapT *bitmap, const Area2D *area, u_short color) {
  short i;

  /* Setup programs word masks/modulo once for the requested rectangle. */
  BlitterSetAreaSetup(bitmap, area);
  for (i = 0; i < bitmap->depth; i++) {
    /* A planar color is split into per-plane bits:
     *  - bit i = 1 => write all ones on plane i
     *  - bit i = 0 => write zeros on plane i
     * This is much faster than CPU-side per-pixel writes on OCS/ECS. */
    BlitterSetAreaStart(i, (color & (1 << i)) ? -1 : 0);
  }
}
