/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <sprite.h>

/*
 * EndSprite — append sprite list terminator (two zero words); advance *datp.
 *
 * DMA stops at 0,0 — required before starting another sprite channel in same buffer.
 */
void EndSprite(SprDataT **datp) {
  u_int *dat = (u_int *)*datp;
  *dat++ = 0;
  *datp = (SprDataT *)dat;
}
