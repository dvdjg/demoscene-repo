/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
/*
 * CopSetupBitplanes — Añade a la lista copper los MOVE que cargan los punteros
 * de bitplanes (bplpt) y el modulo (bpl1mod, bpl2mod). Devuelve el puntero a los
 * pares de instrucciones para poder actualizarlos cada frame en doble buffer.
 */
/*
 * English: Emits CopMove32 for each bplpt[i] up to depth planes; sets bpl1mod/bpl2mod
 * for interleaved (AGA-style) stride or 0 for planar. Returns bplptr handle for
 * CopInsSet32/CopInsSet16 updates each frame (double buffering).
 */
#include <copper.h>

CopInsPairT *CopSetupBitplanes(CopListT *list, const BitmapT *bitmap,
                               u_short depth)
{
  CopInsPairT *bplptr = CopInsPtr(list);

  short modulo = 0;
  short n = depth - 1;

  if (bitmap->flags & BM_INTERLEAVED)
    modulo = (short)bitmap->bytesPerRow * n;

  {
    void *const *planes = bitmap->planes;
    int i = 0;

    do {
      CopMove32(list, bplpt[i++], *planes++);
    } while (--n != -1);
  }

  CopMove16(list, bpl1mod, modulo);
  CopMove16(list, bpl2mod, modulo);
  return bplptr;
}
