/*
 * System API: serial — see system/*.c for implementation.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#ifndef __SYSTEM_SERIAL_H__
#define __SYSTEM_SERIAL_H__

#include <types.h>

struct File;

struct File *OpenSerial(u_int baud, u_int flags);

#endif /* !__SYSTEM_SERIAL_H__ */
