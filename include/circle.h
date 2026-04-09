/*
 * Graphics primitive or display helper (circle.h).
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#ifndef __CIRCLE_H__
#define __CIRCLE_H__

#include "gfx.h"

void Circle(const BitmapT *bitmap, int plane, short x0, short y0, short r);
void CircleEdge(const BitmapT *bitmap, int plane, short x0, short y0, short r);

#endif
