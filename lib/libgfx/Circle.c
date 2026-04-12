/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <circle.h>

/*
 * Plot — set one bit in a bitplane at linear bit index `pos`.
 * NOTE: possible issue: bset immediate expects bit 0–7; `~pos` may not match (pos & 7).
 * Verify against intended pixel addressing if circles look wrong.
 */
static inline void Plot(u_char *pixels, int pos) {
  /*
   * Conceptual C equivalent:
   *   pixels[pos >> 3] |= 1u << (7 - (pos & 7));
   *
   * This code uses bset macro (m68k bit instruction) to reduce instruction count.
   */
  bset(pixels + (pos >> 3), ~pos);
}

/*
 * Circle — integer Bresenham-style circle on one bitplane (CPU, not blitter).
 *
 * d6/d7/a3/a4 fixed regs for inner loop speed; mul16 for row stride. Fills 8 octants
 * from symmetric offsets. C equivalent: same algorithm without register asm.
 */
void Circle(const BitmapT *bitmap, int plane, short x0, short y0, short r) {
  u_char *pixels = bitmap->planes[plane];
  int width = bitmap->bytesPerRow << 3;
  short f = 1 - r;
  short ddF_x = 0;
  short ddF_y = -(r << 1);
  register int x __ASM_REG_PARM("d6") = 0;
  register int y __ASM_REG_PARM("d7") = r;
  register int q0 __ASM_REG_PARM("a3");
  register int q1 __ASM_REG_PARM("a4");
  int q2;
  int q3;

  {
    /* Base point and vertical radius offset in linear-bit coordinates. */
    int base = mul16(y0, width) + x0;
    int yr = mul16(r, width);

    q0 = base + yr;
    q1 = base - yr;
    q2 = base;
    q3 = base;
  }

  Plot(pixels, q0);
  Plot(pixels, q1);
  Plot(pixels, q2 + r);
  Plot(pixels, q3 - r);

  /*
   * Midpoint circle loop:
   * - f is decision variable,
   * - ddF_x / ddF_y are first-order deltas.
   * Each iteration plots symmetric points in all octants.
   */
  while (x < y) {
    if (f >= 0) {
      y--;
      q0 -= width;
      q1 += width;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    q2 += width;
    q3 -= width;
    ddF_x += 2;
    f += ddF_x + 1;    

    Plot(pixels, q0 + x);
    Plot(pixels, q0 - x);
    Plot(pixels, q1 + x);
    Plot(pixels, q1 - x);

    Plot(pixels, q2 + y);
    Plot(pixels, q2 - y);
    Plot(pixels, q3 + y);
    Plot(pixels, q3 - y);
  }
}
