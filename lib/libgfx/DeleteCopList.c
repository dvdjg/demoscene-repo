/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <debug.h>
#include <copper.h>
#include <system/memory.h>

/*
 * DeleteCopList — free CHIP buffer; optional Panic when finished and curr matches
 * 0xfffffffe (same test as original — verify vs your copper builder if this trips).
 */
void DeleteCopList(CopListT *list) {
  if (list->finished && *(u_int *)list->curr == 0xfffffffe)
    Panic("[CopList] End instruction was damaged!");
  MemFree(list);
}
