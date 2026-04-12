/*
 * Compression or integrity check (lzsa.h) for packed demo data.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#ifndef __LZSA_H__
#define __LZSA_H__

void lzsa_depack_stream(const void *input __ASM_REG_PARM("a0"), void *output __ASM_REG_PARM("a2"));

#endif /* !__LZSA_H__ */
