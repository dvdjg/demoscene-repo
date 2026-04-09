/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <copper.h>

/*
 * CopLoadColor — raw-write copper MOVEs for COLOR registers from index start..end.
 *
 * Emits register addresses (CSREG) and color words into the list buffer without
 * going through CopMove16 helper (hand-packed for speed/size).
 * NOTE: possible issue: prototype uses `short color` but body indexes color[start];
 * call sites use CopLoadColor(cp,0,15,0) — confirm intended API vs CopLoadColorArray.
 */
CopInsT *CopLoadColor(CopListT *list, short start, short end, short color) {
  CopInsT *ptr = list->curr;
  u_short *ins = (u_short *)ptr;
  short n = end - start - 1;
  short reg = CSREG(color[start]);

  do {
    *ins++ = reg;
    *ins++ = color;
    reg += 2;
  } while (--n != -1);

  list->curr = (CopInsT *)ins;
  return ptr;
}
