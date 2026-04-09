/*
 * Minimal libc/runtime piece for bare-metal (string/strlen.S).
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
# This code is covered by BSD license. It was taken from NetBSD
# common/lib/libc/arch/m68k/string/strlen.S

#include <asm.h>

# [a0] string
ENTRY(strlen)
        /*
         * Trick:
         *   d0 = ~start_ptr;
         *   while (*p++) ;
         *   return p + d0;
         *
         * Since d0 starts as bitwise-not(pointer), adding final pointer yields
         * byte distance to terminating NUL without keeping a separate counter.
         */
        move.l  a0,d0
        not.l   d0
.Loop:  tst.b   (a0)+           /* null? */
        jne     .Loop           /* no, keep going */
        add.l   a0,d0
        rts
END(strlen)

# vim:ft=gas:ts=8:sw=8:noet:
