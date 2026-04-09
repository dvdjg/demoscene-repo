/*
 * System API: mouse — see system/ *.c for implementation.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#ifndef __SYSTEM_MOUSE_H__
#define __SYSTEM_MOUSE_H__

#include <gfx.h>

#define LMB_PRESSED  1
#define RMB_PRESSED  2
#define LMB_RELEASED 4
#define RMB_RELEASED 8

void MouseInit(Box2D *win);
void MouseKill(void);

#endif /* !__SYSTEM_MOUSE_H__ */
