/*
 * System API: floppy — see system/ *.c for implementation.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#ifndef __SYSTEM_FLOPPY_H__
#define __SYSTEM_FLOPPY_H__

struct File;

#define RAW_TRACK_SIZE 12800
#define SECTOR_SIZE 512
#define NSECTORS 11
#define NTRACKS 160
#define TRACK_SIZE (NSECTORS * SECTOR_SIZE)

struct File *FloppyOpen(int num);

#endif /* !__SYSTEM_FLOPPY_H__ */
