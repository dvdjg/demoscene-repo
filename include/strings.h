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

void bcopy(const void *src __ASM_REG_PARM("a0"), void *dst __ASM_REG_PARM("a1"),
           size_t len __ASM_REG_PARM("d1"));
void bzero(void *s __ASM_REG_PARM("a0"), size_t n __ASM_REG_PARM("d1"));

#endif
