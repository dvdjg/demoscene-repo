/*
 * Compression or integrity check (inflate.h) for packed demo data.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#ifndef __INFLATE_H__
#define __INFLATE_H__

void Inflate(const void *input asm("a4"), void *output asm("a5"));

#endif
