/*
 * Compression or integrity check (checksum.h) for packed demo data.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#ifndef __CHECKSUM_H__
#define __CHECKSUM_H__

#include <types.h>

u_int Checksum(u_int cksum __ASM_REG_PARM("d0"), u_int *data __ASM_REG_PARM("a0"), ssize_t size __ASM_REG_PARM("d1"));

#endif /* !__CHECKSUM_H__ */
