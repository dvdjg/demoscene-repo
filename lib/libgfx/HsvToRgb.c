/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <color.h>

/* Based on https://stackoverflow.com/a/14733008 */
/*
 * HsvToRgb — convert HSV to Amiga OCS RGB12 (4 bits per gun in u_short).
 *
 * h,s,v in author’s scaled units; grays when s==0. Output packs nibbles for COLORxx.
 * Pure CPU; used for procedural fades and plasma-style palettes.
 */
u_short HsvToRgb(short h, short s, short v) {
  short region, remainder, p, q, t;
  u_char r, g, b;

  if (s == 0) {
    v &= 0xf0;
    return (v << 4) | v | (v >> 4);
  }

  region = h / 43;
  remainder = (h - (region * 43)) * 6;

  p = (v * (short)(255 - s)) >> 8;
  q = (v * (short)(255 - ((s * remainder) >> 8))) >> 8;
  t = (v * (short)(255 - ((s * (short)(255 - remainder)) >> 8))) >> 8;

  switch (region) {
    case 0:
      r = v;
      g = t;
      b = p;
      break;
    case 1:
      r = q;
      g = v;
      b = p;
      break;
    case 2:
      r = p;
      g = v;
      b = t;
      break;
    case 3:
      r = p;
      g = q;
      b = v;
      break;
    case 4:
      r = t;
      g = p;
      b = v;
      break;
    default:
      r = v;
      g = p;
      b = q;
      break;
  }

  r &= 0xf0;
  g &= 0xf0;
  b &= 0xf0;

  return (r << 4) | g | (b >> 4);
}
