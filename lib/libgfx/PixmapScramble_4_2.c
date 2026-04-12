/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <pixmap.h>

/*
 * PixmapScramble_4_2 — alternate c2p nibble layout for 4bpp chunky (PM_CMAP4).
 *
 * Masks 0xc3c3 / 0x0c0c group pairs of bits; shifts by 2 swap 2×2 bit blocks.
 * Complements PixmapScramble_4_1 for different blitter fetch patterns.
 */
void PixmapScramble_4_2(const PixmapT *pixmap) {
  if (pixmap->type == PM_CMAP4) {
    u_int *data = pixmap->pixels;
    short n = pixmap->width * pixmap->height / 8;
    register u_int m0 __ASM_REG_PARM("d6") = 0xc3c3c3c3;
    register u_int m1 __ASM_REG_PARM("d7") = 0x0c0c0c0c;

    /* [a0 a1 a2 a3 b0 b1 b2 b3] => [a0 a1 b0 b1 a2 a3 b2 b3] */
    while (--n >= 0) {
      u_int c = *data;
      *data++ = (c & m0) | ((c >> 2) & m1) | ((c & m1) << 2);
    }
  }
}
