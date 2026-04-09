/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <sprite.h>

/*
 * MakeSprite — carve a SpriteT header from a CHIP buffer; advance *datp past sprite data.
 *
 * pos/ctl: SPRPOS/SPRCTL macros pack VSTART/HSTART/VSTOP and height (see sprite.h).
 * attached: true for 15-color attached pair (odd sprite uses previous even’s data).
 * Returns pointer to header; caller fills spr->data[] rows afterward.
 */
SpriteT *MakeSprite(SprDataT **datp, short height, bool attached) {
  SpriteT *spr = (SpriteT *)*datp;
  *datp = &spr->data[height];
  spr->pos = SPRPOS(0, 0);
  spr->ctl = SPRCTL(0, 0, attached, height);
  return spr;
}
