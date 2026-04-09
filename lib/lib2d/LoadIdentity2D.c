/*
 * 2D geometry helpers (lib2d): clipping, transforms.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <2d.h>
#include <fx.h>

/*
 * LoadIdentity2D — 2×2 rotation/scale part = I (fx12f(1.0)), translation = 0.
 *
 * Matrix2D stores fixed-point coeffs; fx12f(1.0) is 1.0 in 12.4 style scaling.
 */
void LoadIdentity2D(Matrix2D *M) {
  M->m00 = fx12f(1.0);
  M->m01 = 0;
  M->x = 0;

  M->m10 = 0;
  M->m11 = fx12f(1.0);
  M->y = 0;
}
