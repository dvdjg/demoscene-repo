/*
 * Software 3D math (lib3d): transforms, sorting — CPU-side; drawing uses blitter/copper.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <3d.h>
#include <fx.h>

void Scale3D(Matrix3D *M, short sx, short sy, short sz) {
  short *m = &M->m00;
  short r;

  r = normfx((*m) * sx); *m++ = r;
  r = normfx((*m) * sy); *m++ = r;
  r = normfx((*m) * sz); *m++ = r;
  m++;

  r = normfx((*m) * sx); *m++ = r;
  r = normfx((*m) * sy); *m++ = r;
  r = normfx((*m) * sz); *m++ = r;
  m++;

  r = normfx((*m) * sx); *m++ = r;
  r = normfx((*m) * sy); *m++ = r;
  r = normfx((*m) * sz); *m++ = r;
}
