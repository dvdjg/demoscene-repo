/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <sprite.h>

CopInsPairT *CopSetupSprites(CopListT *list) {
  SpriteT *spr = NullSprData;
  CopInsPairT *sprptr = CopInsPtr(list);
  short n = 8;
  int i = 0;

  while (--n >= 0)
    CopMove32(list, sprpt[i++], spr);

  return sprptr;
}
