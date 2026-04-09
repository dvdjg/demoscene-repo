/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <sprite.h>

/*
 * SpriteUpdatePos — rewrite SPRxPOS/SPRxCTL words in a SpriteT (raw struct overlay).
 *
 * hstart/vstart: beam position in CopWait coordinates (see beampos.h). Height from
 * SpriteHeight(spr) drives VSTOP. Low bits of HSTART split across POS and CTL per HRM.
 * Writes bytes in hardware order — not the SPRPOS/SPRCTL macros (direct field poke).
 */
void SpriteUpdatePos(SpriteT *spr, hpos hstart, vpos vstart) {
  u_char *raw = (u_char *)spr;
  short hs = hstart.hpos;
  short vs = vstart.vpos;
  short height = SpriteHeight(spr);

  /*
   * SPRxPOS:
   *  Bits 15-8 contain the low 8 bits of VSTART
   *  Bits 7-0 contain the high 8 bits of HSTART
   */

  *raw++ = vs;
  *raw++ = (u_short)hs >> 1;
 
  /*
   * SPRxCTL:
   *  Bits 15-8       The low eight bits of VSTOP
   *  Bit 7           (Used in attachment)
   *  Bits 6-3        Unused (make zero)
   *  Bit 2           The VSTART high bit
   *  Bit 1           The VSTOP high bit
   *  Bit 0           The HSTART low bit
   */

  {
    u_short vstop = vs + height;
    u_char lowctl = hs & 1;

    *raw++ = vstop;

    lowctl |= *raw & 0x80;
    if (vs >= 0x100)
      lowctl += 4;
    if (vstop >= 0x100)
      lowctl += 2;
    *raw++ = lowctl;
  }
}
