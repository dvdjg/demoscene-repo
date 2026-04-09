/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <copper.h>

/*
 * CopLoadColorArray — pack raw MOVE pairs (addr, value) for COLORxx into copper list.
 *
 * Unlike CopLoadColor (broken prototype), this takes a proper `colors[]` and count.
 * CSREG(color[start]) is first register address; +2 per step for consecutive COLOR regs.
 */
CopInsT *CopLoadColorArray(CopListT *list, const u_short *colors, short count,
                           int start) {
  CopInsT *ptr = list->curr;
  u_short *ins = (u_short *)ptr;
  short reg = CSREG(color[start]);
  short n = min((short)count, (short)(32 - start)) - 1;

  do {
    *ins++ = reg;
    *ins++ = *colors++;
    reg += 2;
  } while (--n != -1);

  list->curr = (CopInsT *)ins;
  return ptr;
}
