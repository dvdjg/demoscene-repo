/*
 * Compression or integrity check (zx0.h) for packed demo data.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#ifndef __ZX0_H__
#define __ZX0_H__

void zx0_decompress(const void *input __ASM_REG_PARM("a0"), void *output __ASM_REG_PARM("a1"));

#endif /* !__ZX0_H__ */
