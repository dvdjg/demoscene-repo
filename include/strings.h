/*
 * Header: strings.h — see includes and call sites in the repo.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#ifndef __STRINGS_H__
#define __STRINGS_H__

#include <types.h>

void bcopy(const void *src asm("a0"), void *dst asm("a1"),
           size_t len asm("d1"));
void bzero(void *s asm("a0"), size_t n asm("d1"));

#endif
