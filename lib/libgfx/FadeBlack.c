/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <color.h>
#include <custom.h>

/*
 * FadeBlack — write COLOR registers directly from `colors` with colortab blend toward black.
 *
 * `start` is first COLOR index; `step` 0..15 selects position in colortab (fade amount).
 * `reg` walks custom->color[] — immediate CPU poke; no copper list. Use between frames
 * or during transitions when copper is not driving those same entries.
 */
void FadeBlack(const u_short *colors, short count, u_int start, short step) {
  volatile u_short *reg = &custom->color[start];
 
  if (step < 0)
    step = 0;
  if (step > 15)
    step = 15;

  while (--count >= 0) {
    short to = *colors++;

    short r = ((to >> 4) & 0xf0) | step;
    short g = (to & 0xf0) | step;
    short b = ((to << 4) & 0xf0) | step;

    r = colortab[r];
    g = colortab[g];
    b = colortab[b];

    *reg++ = (r << 4) | g | (b >> 4);
  }
}
