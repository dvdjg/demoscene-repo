/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <copper.h>

/*
 * CopUpdateBitplanes — patch first `n` BPLxPT pairs after CopSetupBitplanes (double buffer).
 *
 * Writes new plane pointers into existing copper MOVEs without rebuilding the list — cheap
 * per-frame update when only bitmap addresses change (scroll, page flip).
 */
void CopUpdateBitplanes(CopInsPairT *bplptr, const BitmapT *bitmap, short n) {
  void *const *planes = bitmap->planes;

  while (--n >= 0)
    CopInsSet32(bplptr++, *planes++);
}
