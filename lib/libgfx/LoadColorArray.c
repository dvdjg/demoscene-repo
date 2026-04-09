/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <custom.h>
#include <palette.h>

/*
 * LoadColorArray — CPU poke of COLOR00..COLOR31 (subset from `start`, max 32 registers).
 *
 * Direct write to custom->color[] — immediate effect same frame; no copper list.
 * Clamps count so start+count ≤ 32. Used for quick palette loads from disk or tables.
 */
void LoadColorArray(const u_short *colors, short count, int start) {
  short n = min((short)count, (short)(32 - start)) - 1;
  volatile u_short *colreg = custom->color + start;

  do {
    *colreg++ = *colors++;
  } while (--n != -1);
}
