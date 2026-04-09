/*
 * Software 3D math (lib3d): transforms, sorting — CPU-side; drawing uses blitter/copper.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <3d.h>
#include <system/memory.h>

void DeleteObject3D(Object3D *object) {
  if (object) {
    MemFree(object->visibleFace);
    MemFree(object);
  }
}
