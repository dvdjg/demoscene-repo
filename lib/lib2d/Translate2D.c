/*
 * 2D geometry helpers (lib2d): clipping, transforms.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <2d.h>

void Translate2D(Matrix2D *M, short x, short y) {
  M->x += x;
  M->y += y;
}
