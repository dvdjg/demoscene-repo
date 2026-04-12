/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <line.h>

/*
 * CpuLine implementation notes:
 *
 * - Algorithm: integer Bresenham (error accumulator `dg`) with octant split into
 *   4 cases (major axis X/Y + direction left/right).
 * - Pixel format: 1 bitplane packed as 16 pixels per u_short write window.
 *   `color` is a rolling bit mask (one hot bit) inside current 16-bit word.
 * - Why inline asm helps:
 *   * `rolw/rorw` updates bit mask and carry flag in one instruction.
 *   * carry flag directly tells when mask crossed word boundary, so pointer can
 *     be adjusted by ±2 bytes without extra compare/branch sequence.
 *   * Register pinning (`d0..d3`, `a0`) avoids spills in the hot loop.
 *
 * A theoretically perfect optimizing C compiler could approach this, but on m68k
 * toolchains used here, the explicit asm pattern is consistently tighter and more
 * predictable in cycles/branches.
 */

static struct {
  /* Base address of selected destination bitplane. */
  u_char *pixels;
  /* Bytes per row in that bitplane. */
  int stride;
} line;

/* CpuLineSetup — bind destination plane once, then CpuLine can run branch-light. */
void CpuLineSetup(const BitmapT *bitmap, u_short plane) {
  line.pixels = bitmap->planes[plane];
  line.stride = bitmap->bytesPerRow;
}

/*
 * CpuLine — draw one 1bpp line using CPU Bresenham + bit-mask rotation.
 *
 * Register binding:
 * - d0..d3 carry endpoints (xs,ys,xe,ye) for call ABI friendliness and speed.
 * - a0 holds running destination pointer (`pixels`) in inner loops.
 */
void CpuLine(short xs __ASM_REG_PARM("d0"), short ys __ASM_REG_PARM("d1"),
             short xe __ASM_REG_PARM("d2"), short ye __ASM_REG_PARM("d3"))
{
  void *pixels = line.pixels;
  short stride = line.stride;
  /* One-hot bit inside current destination word (bit 15 = leftmost pixel). */
  u_short color;
  short dx, dy;

  /* Normalize so we always advance downward in Y (reduces octants from 8 to 4). */
  if (ys > ye) {
    swapr(xs, xe);
    swapr(ys, ye);
  }

  /* Move to starting row and 16-bit word containing xs. */
  pixels += ys * stride;
  pixels += (xs >> 3) & -2;

  /* Seed mask from xs modulo 16. */
  color = 0x8000 >> (xs & 15);

  dy = ye - ys;
  dx = abs(xe - xs);

  /* Major axis = Y (steep line). */
  if (dx < dy) {
    short dg2 = 2 * dx;
    short dg = dg2 - dy;
    short dg1 = 2 * dy;

    dg -= dg2; /* precompensate for [dg += dg2] */
    dy--;

    if (xe < xs) {
      /* Case 1: steep + going left. */
      do {
        *(u_short *)pixels |= color;

        dg += dg2;

        if (dg >= 0) {
          dg -= dg1;
          /* rolw shifts mask left; carry means wrap past bit 15 -> previous word. */
          /*
           * Equivalent C intent (conceptual):
           *   bool wrapped = (color & 0x8000) != 0;
           *   color = (u_short)((color << 1) | (wrapped ? 1 : 0));
           *   if (wrapped) pixels -= 2;
           *
           * Why asm here:
           * - 68000 `rolw` already computes wrapped bit in C flag "for free".
           * - `bccs` branches directly on that flag without extra test/compare.
           * - avoids extra instructions that C codegen usually emits for wrap detect.
           */
          asm("rolw  #1,%0\n"
              "bccs  . +4\n"
              "subql #2,%1\n" : "+d" (color), "+a" (pixels));
        }

        pixels += stride;
      } while (--dy != -1);
    } else {
      /* Case 2: steep + going right. */
      do {
        *(u_short *)pixels |= color;

        dg += dg2;

        if (dg >= 0) {
          /* rorw shifts mask right; carry means wrap past bit 0 -> next word. */
          /*
           * Equivalent C intent (conceptual):
           *   bool wrapped = (color & 0x0001) != 0;
           *   color = (u_short)((color >> 1) | (wrapped ? 0x8000 : 0));
           *   if (wrapped) pixels += 2;
           *
           * Why asm wins:
           * - `rorw` + carry flag gives rotate+wrap detection in one instruction.
           * - pointer bump piggybacks on branch-on-carry, minimal branch pressure.
           */
          asm("rorw  #1,%0\n"
              "bccs  . +4\n"
              "addql #2,%1\n" : "+d" (color), "+a" (pixels));
          dg -= dg1;
        }

        pixels += stride;
      } while (--dy != -1);
    }
  } else {
    /* Major axis = X (shallow line). */
    short dg2 = 2 * dy;
    short dg = dg2 - dx;
    short dg1 = 2 * dx;

    dg -= dg2; /* precompensate for [dg += dg2] */
    dy--;

    if (xe < xs) {
      /* Case 3: shallow + going left. */
      do {
        *(u_short *)pixels |= color;

        dg += dg2;
        if (dg >= 0) {
          dg -= dg1;
          /* Secondary step in Y when error crosses threshold. */
          pixels += stride;
        }

        asm("rolw  #1,%0\n"
            "bccs  . +4\n"
            "subql #2,%1\n" : "+d" (color), "+a" (pixels));
        /*
         * Same rotate-left primitive as case 1.
         * This stays in asm (instead of helper C function) to keep the inner loop
         * branch+ALU sequence short and register-only on m68k.
         */
      } while (--dx != -1);
    } else {
      /* Case 4: shallow + going right. */
      do {
        *(u_short *)pixels |= color;

        dg += dg2;
        if (dg >= 0) {
          dg -= dg1;
          pixels += stride;
        }

        asm("rorw  #1,%0\n"
            "bccs  . +4\n"
            "addql #2,%1\n" : "+d" (color), "+a" (pixels));
        /*
         * Same rotate-right primitive as case 2; branch-on-carry encodes
         * "crossed 16-bit word boundary?" with no explicit C-level condition.
         */
      } while (--dx != -1);
    }
  }
}
