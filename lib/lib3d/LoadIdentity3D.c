/*
 * Software 3D math (lib3d): transforms, sorting — CPU-side; drawing uses blitter/copper.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <string.h>
#include <3d.h>
#include <fx.h>

void LoadIdentity3D(Matrix3D *M) {
  memset(M, 0, sizeof(Matrix3D));
  M->m00 = fx12f(1.0);
  M->m11 = fx12f(1.0);
  M->m22 = fx12f(1.0);
}
