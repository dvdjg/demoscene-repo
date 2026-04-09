/*
 * Compression or integrity check (crc32.h) for packed demo data.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#ifndef __CRC32_H__
#define __CRC32_H__

#include <types.h>

u_int crc32(const u_char *frame, size_t frame_len);

#endif /* __CRC32_H__ */
