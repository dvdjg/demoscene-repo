/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <pixmap.h>

/*
 * PixmapScramble_4_1 — reorder 4bpp chunky nibbles for c2p / blitter interleave (PM_CMAP4).
 *
 * Masks m0/m1 split odd/even bit lanes; shifts swap nibbles into plane-friendly layout.
 * d6/d7 hold constants across the loop. CPU-bound; run once after filling pixmap.
 */
void PixmapScramble_4_1(const PixmapT *pixmap) {
  if (pixmap->type == PM_CMAP4) {
    u_int *data = pixmap->pixels;
    short n = pixmap->width * pixmap->height / 8;
    register u_int m0 asm("d6") = 0xa5a5a5a5;
    register u_int m1 asm("d7") = 0x0a0a0a0a;

    /* [a0 a1 a2 a3 b0 b1 b2 b3] => [a0 b0 a2 b1 a1 b2 a3 b3] */
    while (--n >= 0) {
      u_int c = *data;
      *data++ = (c & m0) | ((c >> 3) & m1) | ((c & m1) << 3);
    }
  }
}
