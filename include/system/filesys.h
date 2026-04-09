/*
 * System API: filesys — see system/*.c for implementation.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#ifndef __SYSTEM_FILESYS_H__
#define __SYSTEM_FILESYS_H__

#include <types.h>

struct File;

#ifdef _SYSTEM
void InitFileSys(struct File *dev);
void CheckFileSys(void);
void KillFileSys(void);
#endif

struct File *OpenFile(const char *path);

#endif /* !__SYSTEM_FILESYS_H__ */
