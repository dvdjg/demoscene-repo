/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <debug.h>
#include <copper.h>

/*
 * CopListActivate — point COP1LC at list->entry and enable copper + master DMA.
 *
 * cop1lc is the first copper list pointer (OCS); COP2LC exists for AGA second list.
 * WaitVBlank: avoids mid-frame switch tearing when replacing the whole list.
 * C equivalent: `(custom->cop1lc = (u_int)addr); EnableDMA(DMAF_MASTER|DMAF_COPPER);`
 */
void CopListActivate(CopListT *list) {
  if (list->finished == 0)
    Panic("[CopList] Cannot activate list that is not finished!");
  /* Write copper list address. */
  custom->cop1lc = (u_int)list->entry;
  /* Enable copper DMA */
  EnableDMA(DMAF_MASTER | DMAF_COPPER);
  /* Wait for vertical blank to make sure the list is active. */
  WaitVBlank();
}
