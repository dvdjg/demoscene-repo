/*
 * Software 3D math (lib3d): transforms, sorting — CPU-side; drawing uses blitter/copper.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <3d.h>

void AllFacesDoubleSided(Object3D *object) {
  void *_objdat = object->objdat;
  short *group = object->faceGroups;
  short f;

  do {
    while ((f = *group++)) {
      FACE(f)->material |= 0x80;
    }
  } while (*group);
}
