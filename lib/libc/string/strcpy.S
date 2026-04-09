/*
 * Minimal libc/runtime piece for bare-metal (string/strcpy.S).
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
# This code is covered by BSD license. It was taken from NetBSD
# common/lib/libc/arch/m68k/string/strlen.S

#include <asm.h>

# [a0] fromaddr
# [a1] toaddr
ENTRY(strcpy)
        move.l  a1,d0
.Loop:  move.b  (a0)+,(a1)+     /* copy a byte */
        jne     .Loop           /* copied non-null, keep going */
        rts
END(strcpy)

# vim: ft=gas:ts=8:sw=8:noet:
