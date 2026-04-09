/*
 * Blitter-oriented code (libblit): DMA-friendly operations.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <blitter.h>

/*
 * FirstWordMask[i] — for blits starting at pixel offset i (0–15) within a word:
 * mask of valid bits from first pixel to end of word (i high bits cleared if i>0).
 *
 * LastWordMask[w] — mask for last partial word when width mod 16 = w.
 * Used as BLTAFWM / BLTALWM so the blitter does not write outside the rectangle.
 * C equivalent: (0xFFFF >> (i & 15)) for first word (conceptually); tables are exact HRM.
 */
const u_short FirstWordMask[16] = {
  0xFFFF, 0x7FFF, 0x3FFF, 0x1FFF, 0x0FFF, 0x07FF, 0x03FF, 0x01FF,
  0x00FF, 0x007F, 0x003F, 0x001F, 0x000F, 0x0007, 0x0003, 0x0001
};

const u_short LastWordMask[16] = {
  0xFFFF, 0x8000, 0xC000, 0xE000, 0xF000, 0xF800, 0xFC00, 0xFE00,
  0xFF00, 0xFF80, 0xFFC0, 0xFFE0, 0xFFF0, 0xFFF8, 0xFFFC, 0xFFFE
};
