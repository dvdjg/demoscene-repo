/*
 * Software 3D math (lib3d): transforms, sorting — CPU-side; drawing uses blitter/copper.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <strings.h>
#include <3d.h>

void UpdateVertexVisibility(Object3D *object) {
  register char s __ASM_REG_PARM("d3") = 1;

  void *_objdat = object->objdat;
  register short *group __ASM_REG_PARM("a2") = object->faceGroups;
  short f;

  do {
    while ((f = *group++)) {
      if (FACE(f)->flags >= 0) {
        register FaceIndexT *index __ASM_REG_PARM("a3") = FACE(f)->indices;
        short vertices = FACE(f)->count - 3;
        short i;

        /* Face has at least (and usually) three vertices / edges. */
        i = index->vertex; index++; NODE3D(i)->flags = s;
        i = index->vertex; index++; NODE3D(i)->flags = s;

        do {
          i = index->vertex; index++; NODE3D(i)->flags = s;
        } while (--vertices != -1);
      }
    }
  } while (*group);
}
