/*
 * __umodsi3 — unsigned remainder for 32-bit `%` (gen/umodsi3.S).
 *
 * Purpose: GCC lowers `uint32_t % uint32_t` to this helper. `__udivsi3` is
 * shared with signed division: it returns the quotient in `d0` but leaves the
 * remainder in `d1` for the unsigned inputs we passed on the stack.
 *
 * This file is a tiny wrapper around `bsr __udivsi3` because the quotient
 * (returned in `d0`) is discarded — we only need `d1`.
 *
 * Conceptual C:
 *
 *   uint32_t __umodsi3(uint32_t a, uint32_t b) {
 *     uint32_t q = __udivsi3(a, b);
 *     (void)q;
 *     return remainder; // same value the hardware left in d1
 *   }
 *
 * ABI contract:
 * - `__udivsi3` must preserve `a0` because we pop the return address into it
 *   before `bsr` and then `jmp (a0)` instead of `rts` (saves a stack slot).
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
/* Based on umodsi3.S from NetBSD */

#include <asm.h>

ENTRY(__umodsi3)
	/*
	 * NB: this requires that __udivsi3 preserve the a0
         * register, and that it returns the modulus in d1.
	 */
	move.l	(sp)+,a0	/* return address in a0 — not left on stack for rts */
	bsr	_L(__udivsi3)
	move.l	d1,d0		/* C return value lives in d0 */
	jmp	(a0)		/* tail-return to caller */
END(__umodsi3)

# vim: ft=gas:ts=8:sw=8:noet:
