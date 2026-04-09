/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <copper.h>

/*
 * CopListReset — rewind builder cursor to start; clear overflow flag.
 * Use before reusing a CopListT buffer for a new program (same allocation).
 */
CopListT *CopListReset(CopListT *list) {
  list->curr = list->entry;
  list->overflow = 0;
  return list;
}
