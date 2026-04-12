/*
 * Header: string.h — see includes and call sites in the repo.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#ifndef __STRING_H__
#define __STRING_H__

#include <types.h>

void *memmove(void *dst __ASM_REG_PARM("a1"), const void *src __ASM_REG_PARM("a0"),
              size_t len __ASM_REG_PARM("d1"));
void *memset(void *b __ASM_REG_PARM("a0"), int c __ASM_REG_PARM("d0"), size_t len __ASM_REG_PARM("d1"));
void *memcpy(void *__restrict dst __ASM_REG_PARM("a1"),
             const void *__restrict src __ASM_REG_PARM("a0"), size_t n __ASM_REG_PARM("d1"));
char *strcpy(char *dst, const char *src);
int strcmp(const char *s1 __ASM_REG_PARM("a0"), const char *s2 __ASM_REG_PARM("a1"));
size_t strlen(const char *s __ASM_REG_PARM("a0"));

size_t strlcpy(char *__restrict dst, const char *__restrict src, size_t siz);

#endif
