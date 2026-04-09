/*
 * Assembler/linker helpers (ENTRY/END) shared by .S and the build.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#ifndef __ASM_H__
#define __ASM_H__

#include <cdefs.h>

#ifdef __ELF__
#define _L(name) name
#else
#define _L(name) __CONCAT(_, name)
#endif

#define ENTRY(name)                                                            \
  .text;                                                                       \
  .even;                                                                       \
  .globl _L(name);                                                             \
  .type _L(name), @function;                                                   \
  _L(name) :

#define END(name) .size _L(name), .- _L(name)

#define STRONG_ALIAS(alias, sym)                                               \
  .globl _L(alias);                                                            \
  _L(alias) = _L(sym)

#endif
