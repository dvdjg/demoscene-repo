/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <sprite.h>

/*
 * SpriteHeight — decode vertical span from raw SPRxPOS/SPRxCTL bytes (VSTART/VSTOP bits).
 *
 * Handles 9-bit VSTART/VSTOP via CTL bits 2/1 (wrap past 255 lines). Matches
 * SpriteUpdatePos packing so move + height queries stay consistent.
 */
short SpriteHeight(SpriteT *spr) {
  u_char *raw = (u_char *)spr;
  short vstart, vstop, height;

  vstart = *raw++;
  raw++;
  vstop = *raw++;

  height = vstop - vstart;
  if (*raw & 4)
    height -= 0x100;
  if (*raw & 2)
    height += 0x100;

  return height;
}
