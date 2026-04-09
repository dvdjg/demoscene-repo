/*
 * Header: c2p_1x1_4.h — see includes and call sites in the repo.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#ifndef __C2P_1x1_4_H__
#define __C2P_1x1_4_H__

/*
 * width: must be even multiple of 16
 * bplSize: offset between one row in one bpl and the next bpl in bytes
 */

void c2p_1x1_4(void *chunky asm("a0"), void *bpls asm("a1"),
               short width asm("d0"), short height asm("d1"),
               int bplSize asm("d5"));

#endif
