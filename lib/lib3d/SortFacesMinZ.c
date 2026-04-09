/*
 * Software 3D math (lib3d): transforms, sorting — CPU-side; drawing uses blitter/copper.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <3d.h>

void SortFacesMinZ(Object3D *object) {
  short *item = (short *)object->visibleFace;
  short count = 0;

  void *_objdat = object->objdat;
  short *group = object->faceGroups;

  do {
    short f;

    while ((f = *group++)) {
      if (FACE(f)->flags >= 0) {
        short minZ = 32767;

        short *index = (short *)&FACE(f)->count;
        short n = (*index++);
        short i;

        for (i = 0; i < n; i++) {
          short j = FACE(f)->indices[i].vertex;
          short z = VERTEX(j)->z;
          if (z < minZ)
            minZ = z;
        }

        *item++ = minZ;
        *item++ = f;
        count++;
      }
    }
  } while (*group);

  /* guard element */
  *item++ = 0;
  *item++ = -1;

  SortItemArray(object->visibleFace, count);
}
