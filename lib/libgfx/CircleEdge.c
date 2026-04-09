/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <circle.h>

/*
 * Plot — set one bit at linear index `pos` (same helper as Circle.c).
 * NOTE: possible issue: bset bit number vs `(pos & 7)` — verify if outline looks wrong.
 */
static inline void Plot(u_char *pixels, int pos) {
  /* Conceptual C:
   *   pixels[pos >> 3] |= 1u << (7 - (pos & 7));
   * bset avoids explicit load/shift/store sequence on 68000. */
  bset(pixels + (pos >> 3), ~pos);
}

/*
 * CircleEdge — outline-only variant of the circle (fewer Plot calls per octant step).
 * `dot` gates the horizontal quadrants when f>0 to thin the stroke vs filled Circle().
 */
void CircleEdge(const BitmapT *bitmap, int plane, short x0, short y0, short r) {
  u_char *pixels = bitmap->planes[plane];
  int width = bitmap->bytesPerRow << 3;
  short f = 1 - r;
  short ddF_x = 0;
  short ddF_y = -(r << 1);
  register int x asm("d6") = 0;
  register int y asm("d7") = r;
  register int q0 asm("a3");
  register int q1 asm("a4");
  int q2;
  int q3;
  u_char dot;

  {
    /* Compute center in linear-bit coordinates and +/- radius offsets. */
    int base = mul16(y0, width) + x0;
    int yr = mul16(r, width);

    q0 = base + yr;
    q1 = base - yr;
    q2 = base;
    q3 = base;
  }

  Plot(pixels, q2 + r);
  Plot(pixels, q3 - r);

  /*
   * Same midpoint decision as Circle(), but draws only contour points.
   * `dot` gates one symmetric quartet when decision step didn't reduce `y`,
   * producing a thinner, cleaner edge.
   */
  while (x < y) {
    if (f > 0) {
      y--;
      q0 -= width;
      q1 += width;
      ddF_y += 2;
      f += ddF_y;
      dot = -1;
    } else {
      dot = 0;
    }

    x++;
    q2 += width;
    q3 -= width;
    ddF_x += 2;
    f += ddF_x + 1;    

    if (dot) {
      Plot(pixels, q0 + x);
      Plot(pixels, q0 - x);
      Plot(pixels, q1 + x);
      Plot(pixels, q1 - x);
    }

    Plot(pixels, q2 + y);
    Plot(pixels, q2 - y);
    Plot(pixels, q3 + y);
    Plot(pixels, q3 - y);
  }
}
