/*
 * 2D geometry helpers (lib2d): clipping, transforms.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
/*
 * Transform2D — Aplica una matriz 2D (con traslación) a n puntos.
 * Optimizado para 68000: punteros a short, producto con normfx (fixed-point 4.12).
 * Usado por formas 2D, animaciones de polígonos y efectos que transforman puntos en pantalla.
 */
/*
 * English: Matrix2D row-major: (m00,m01,mx) then (m10,m11,my). Each output coord is
 * normfx(dot) + translation — same fixed convention as 3D path. Pure CPU; blitter draws later.
 */
#include <2d.h>
#include <fx.h>

/* Transform2D implementation detail:
 * - matrix coefficients are read once into locals (m00..my) to avoid repeated
 *   memory access in the hot loop,
 * - src/dst are traversed as short* for tight pointer arithmetic on m68k,
 * - normfx() converts 32-bit multiply accumulation back to fixed-point domain. */
void Transform2D(Matrix2D *M, Point2D *out, Point2D *in, short n) {
  short *dst = (short *)out;
  short *src = (short *)in;
  short *v = (short *)M;
  short m00 = *v++;
  short m01 = *v++;
  short mx  = *v++;
  short m10 = *v++;
  short m11 = *v++;
  short my  = *v++;

  while (--n >= 0) {
    short x = *src++;
    short y = *src++;

    /* Conceptual C math:
     * out.x = (m00*x + m01*y) / FX_ONE + mx;
     * out.y = (m10*x + m11*y) / FX_ONE + my; */
    *dst++ = normfx(m00 * x + m01 * y) + mx;
    *dst++ = normfx(m10 * x + m11 * y) + my;
  }
}
