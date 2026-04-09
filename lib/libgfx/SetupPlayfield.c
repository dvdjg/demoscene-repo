/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <playfield.h>

/*
 * SetupPlayfield — one-shot: mode/depth, data fetch (DDF), display window (DIW).
 * Order matters: BPLCON0 depth before fetch/window in underlying helpers.
 */
void SetupPlayfield(u_short mode, u_short depth,
                    hpos xs, vpos ys, u_short w, u_short h)
{
  SetupMode(mode, depth);
  SetupBitplaneFetch(mode, xs, w);
  SetupDisplayWindow(mode, xs, ys, w, h);
}
