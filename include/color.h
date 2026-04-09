/*
 * Palette helpers — RGB12 (4 bits per gun) as u_short for COLORxx registers.
 *
 * colortab — optional 4096-entry LUT for fast RGB12 lookups (see lib implementation).
 * ColorTransition — linear blend between two OCS colors in RGB space (step 0..15).
 * HsvToRgb — HSV→RGB12 for procedural palettes. FadeBlack — dim palette toward black.
 * C equivalent: interpolate (r,g,b) separately then pack to 0x0RGB nibbles.
 * HRM "Color Registers": https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 */
#ifndef __COLOR_H__
#define __COLOR_H__

#include "types.h"

extern u_char colortab[4096];

/* from, to, step: each 0..15 for predictable EHB-style transitions. */
u_short ColorTransition(u_short from, u_short to, u_short step);

u_short HsvToRgb(short h, short s, short v);

void FadeBlack(const u_short *colors, short count, u_int start, short step);

#endif
