/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <copper.h>
#include <debug.h>

/*
 * CopListFinish — append copper END (0xFFFF,0xFFFE) and mark list valid.
 *
 * stli writes two words at end of program; copper stops interpreting after END.
 * Logs used slots; Panic if builder wrote past length (overflow).
 */
CopListT *CopListFinish(CopListT *list) {
  CopInsT *ins = list->curr;
  stli(ins, 0xfffffffe);
  list->curr = ins;
  {
    ptrdiff_t used = (ptrdiff_t)(list->curr - list->entry);
    Log("[CopList] %p: used slots %ld/%d\n", list, used, list->length);
    if (used > list->length)
      Panic("[CopList] Overflow detected!");
  }
  list->finished = -1;
  return list;
}
