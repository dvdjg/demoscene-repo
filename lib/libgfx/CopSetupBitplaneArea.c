/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <copper.h>

/*
 * CopSetupBitplaneArea — point BPLxPT at a sub-rectangle of a larger bitmap + set modulo.
 *
 * Used for scrolling windows: `start` offsets into CHIP plane data; modulo reduces
 * effective fetch width to `w`. Minimum width 32 pixels (two words) — hardware quirk.
 * x.hpos adjusted by sub-word offset for fine horizontal alignment (BPLCON1 elsewhere).
 * Delegates fetch timing to CopSetupBitplaneFetch.
 */
void CopSetupBitplaneArea(CopListT *list, u_short mode, u_short depth,
                          const BitmapT *bitmap, hpos x, vpos y __unused,
                          const Area2D *area)
{
  void *const *planes = bitmap->planes;
  int start;
  short modulo;
  short w;
  short i;

  if (area) {
    w = (area->w + 15) & ~15;
    /* Seems that bitplane fetcher has to be active for at least two words! */
    if (w < 32)
      w = 32;
    start = bitmap->bytesPerRow * area->y + ((area->x >> 3) & ~1);
    modulo = bitmap->bytesPerRow - ((w >> 3) & ~1);
    x.hpos -= (area->x & 15);
  } else {
    w = (bitmap->width + 15) & ~15;
    start = 0;
    modulo = 0;
  }

  for (i = 0; i < depth; i++)
    CopMove32(list, bplpt[i], *planes++ + start);

  CopMove16(list, bpl1mod, modulo);
  CopMove16(list, bpl2mod, modulo);

  CopSetupBitplaneFetch(list, mode, x, w);
}
