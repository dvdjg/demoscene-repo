/*
 * Software 3D math (lib3d): transforms, sorting — CPU-side; drawing uses blitter/copper.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
/*
 * Transform3D — Aplica una matriz 3×3 + traslación a n puntos 3D.
 * Cada punto (x,y,z) se multiplica por M y se suma la traslación; resultado en fixed-point
 * con normfx. Usado para pasar vértices de espacio objeto a mundo o cámara.
 */
/*
 * English: Matrix3D stores 3 rows of (mx,my,mz,tx) — fourth column is translation.
 * MULVERTEX expands one row: dot(row, (x,y,z)) + translation. Unrolled for speed vs
 * triple nested loop in C. Pure CPU math; no custom chips until projection/raster.
 */
#include <3d.h>
#include <fx.h>

/* MULVERTEX expands one matrix row multiply-accumulate + translation.
 * Kept as macro so compiler can inline row loads and avoid call overhead in the
 * triple-row inner loop. */
#define MULVERTEX() {                 \
  short v0 = (*v++);                  \
  short v1 = (*v++);                  \
  short v2 = (*v++);                  \
  short t3 = (*v++);                  \
  int t0 = v0 * x;                    \
  int t1 = v1 * y;                    \
  int t2 = v2 * z;                    \
  *dst++ = normfx(t0 + t1 + t2) + t3; \
}

/* Transform3D — per-vertex 3x3 + translation transform in fixed-point.
 * Unrolled by rows (MULVERTEX x3) instead of nested for-loops for tighter codegen
 * on classic m68k compilers. */
void Transform3D(Matrix3D *M, Point3D *out, Point3D *in, short n) {
  short *src = (short *)in;
  short *dst = (short *)out;

  while (--n >= 0) {
    short *v = (short *)M;
    short x = *src++;
    short y = *src++;
    short z = *src++;

    /* Row 0 -> out.x, row 1 -> out.y, row 2 -> out.z */
    MULVERTEX();
    MULVERTEX();
    MULVERTEX();

    /* Skip potential padding/extra field in Point3D representation. */
    src++; dst++;
  }
}
