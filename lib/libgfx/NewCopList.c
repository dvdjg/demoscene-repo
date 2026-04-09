/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <copper.h>
#include <system/memory.h>

/*
 * NewCopList — allocate CHIP RAM for `length` CopInsT slots (MOVE/WAIT/SKIP).
 *
 * The copper list must live in CHIP: DMA reads instructions from it every frame.
 * curr points to next free slot; overflow/finished track builder state (see copper.c).
 */
CopListT *NewCopList(int length) {
  CopListT *list =
    MemAlloc(sizeof(CopListT) + length * sizeof(CopInsT), MEMF_CHIP);
  list->length = length;
  list->curr = list->entry;
  list->overflow = 0;
  list->finished = 0;
  return list;
}
