/*
 * Minimal libc/runtime piece for bare-metal (string/strcmp.S).
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
# This code is covered by BSD license. It was taken from NetBSD
# common/lib/libc/arch/m68k/string/strcmp.S

#include <asm.h>

# [a0] s1
# [a1] s2
ENTRY(strcmp)
#if defined(__ELF__)
        move.l 4(%sp),%a0
        move.l 8(%sp),%a1
#endif
.L1:                            /* unrolled by 4 for 680[23]0's */
        /*
         * Unrolled compare body:
         *   c1 = *s1++; if (!c1) goto end_on_nul;
         *   diff = c1 - *s2++; if (diff) goto return_diff;
         * repeated 4x before looping.
         *
         * Why asm: tight byte loads + subtract + branch sequence with minimal
         * register pressure and no compiler-introduced temporaries.
         */
        move.b  (a0)+,d1
        jeq     .L2
        sub.b   (a1)+,d1
        jne     .L3

        move.b  (a0)+,d1
        jeq     .L2
        sub.b   (a1)+,d1
        jne     .L3

        move.b  (a0)+,d1
        jeq     .L2
        sub.b   (a1)+,d1
        jne     .L3

        move.b  (a0)+,d1
        jeq     .L2
        sub.b   (a1)+,d1
        jeq     .L1
.L3:
        /* d1 contains signed byte difference; normalize to int return convention. */
        scs     d0
        ext.w   d0
        ext.l   d0
        move.b  d1,d0
        rts
.L2:
        /* Reached NUL in s1: return -(unsigned char)*s2 (0 if both ended). */
        moveq.l #0,d0
        move.b  (a1),d0
        neg.l   d0
        rts
END(strcmp)

# vim: ft=gas:ts=8:sw=8:noet:
