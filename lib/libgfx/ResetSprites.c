/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <custom.h>
#include <sprite.h>

/*
 * ResetSprites — disable sprite DMA, set playfield/sprite priorities, point all SPRxPT
 * at NullSprData and clear sprite data registers.
 *
 * BPLCON2_PF?P_SP07: sprites in front of both playfields (typical for mouse pointer).
 * NullSprData: safe invisible sprite in CHIP (see NullSprData.c).
 */
void ResetSprites(void) {
  short i;

  DisableDMA(DMAF_SPRITE);

  /* Move sprites into foreground. */
  custom->bplcon2 = BPLCON2_PF1P_SP07 | BPLCON2_PF2P_SP07;

  for (i = 0; i < 8; i++) {
    custom->sprpt[i] = NullSprData;
    custom->spr[i].datab = 0;
    custom->spr[i].dataa = 0;
  }
}
