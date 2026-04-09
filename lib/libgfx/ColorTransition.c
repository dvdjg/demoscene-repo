/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <color.h>

/*
 * ColorTransition — blend `from` toward `to` using per-gun `colortab` lookups.
 *
 * Packs R,G,B nibbles into indices (from/to/step mix); colortab holds precomputed
 * blend curve for OCS 12-bit RGB. step 0..15 selects position along the blend.
 * Output is one u_short COLORxx value. Pure CPU; no copper involvement.
 */
u_short ColorTransition(u_short from, u_short to, u_short step) {
  short r = (from & 0xf00) | ((to >> 4) & 0x0f0) | step;
  short g = ((from << 4) & 0xf00) | (to & 0x0f0) | step;
  short b = ((from << 8) & 0xf00) | ((to << 4) & 0x0f0) | step;

  r = colortab[r];
  g = colortab[g];
  b = colortab[b];

  return (r << 4) | g | (b >> 4);
}
