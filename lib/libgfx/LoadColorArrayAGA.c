/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <custom.h>
#include <palette.h>

void LoadColorArrayAGA(const rgb *colors, short count, int start) {
  short n = min((short)count, (short)(256 - start)) - 1;

  do {
    SetColorAGA(start++, *colors++);
  } while (--n != -1);
}
