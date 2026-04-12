/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <line.h>

static struct {
  /* Base pointer to selected destination bitplane. */
  u_char *pixels;
  /* Bytes per scanline in destination bitplane. */
  int stride;
} edge;

/* CpuEdgeSetup — bind one destination plane so edge rasterizer avoids reload work. */
void CpuEdgeSetup(const BitmapT *bitmap, u_short plane) {
  edge.pixels = bitmap->planes[plane];
  edge.stride = bitmap->bytesPerRow;
}

/*
 * CpuEdge — draw an edge using integer incremental stepping (scan-conversion style).
 *
 * Compared with CpuLine:
 * - this routine toggles one bit (`bchg`) per row and keeps explicit integer/fraction
 *   accumulators (`di` + `df`) from dx/dy decomposition.
 * - suitable for polygon edge walkers where "one sample per Y" behavior matters.
 *
 * Core idea:
 *   adx = abs(dx)
 *   di  = floor(adx / dy)   // whole-pixel horizontal advance per row
 *   df  = adx % dy          // fractional residue accumulated in `xf`
 */
void CpuEdge(short xs __ASM_REG_PARM("d0"), short ys __ASM_REG_PARM("d1"),
             short xe __ASM_REG_PARM("d2"), short ye __ASM_REG_PARM("d3"))
{
  u_char *pixels = edge.pixels;
  short stride = edge.stride;
  /* dx,dy edge delta; di/df are integer and fractional X increments per Y step. */
  short dx, dy, di, df, n, xi, xf;
  int adx;

  /* Normalize to iterate from top to bottom (ys <= ye). */
  if (ys > ye) {
    swapr(xs, xe);
    swapr(ys, ye);
  }

  dx = xe - xs;
  dy = ye - ys;

  if (dy == 0)
    return;

  /* Byte address of start pixel. */
  pixels += ys * stride;
  pixels += xs >> 3;

  adx = absw(dx);

  if (adx < dy) {
    di = 0;
    df = adx;
  } else {
    divmod16(adx, dy, di, df);
  }

  /* xi is bit index inside current byte (inverted because Amiga bit order is MSB-first). */
  xi = ~xs & 7;
  xf = -dy;
  n = dy - 1;

  if (dx >= 0) {
    di = di & 7;
    stride += di >> 3;

    do {
      /*
       * bchg equivalent (conceptual C):
       *   pixels[0] ^= (1u << xi);
       *
       * Why bchg asm macro is preferred:
       * - maps to one m68k instruction with memory bit op semantics,
       * - avoids load/xor/store sequence that C usually emits on 68000.
       */
      bchg(pixels, xi);

      pixels += stride;

      xf += df;
      if (xf >= 0) {
        xi--;
        xf -= dy;
      }

      xi -= di;
      if (xi < 0) {
        pixels++;
        xi += 8;
      }
    } while (--n != -1);
  } else {
    stride -= di >> 3;
    di = -(di & 7);
    xi -= 8;

    do {
      /* Same bit toggle as above; left-going branch mirrors addressing updates. */
      bchg(pixels, xi);

      pixels += stride;

      xf += df;
      if (xf >= 0) {
        xi++;
        xf -= dy;
      }

      xi -= di;
      if (xi >= 0) {
        pixels--;
        xi -= 8;
      }
    } while (--n != -1);
  }
}
