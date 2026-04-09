/*
 * __modsi3 — signed remainder for `%` on 32-bit ints (gen/modsi3.S).
 *
 * Purpose: GCC lowers `a % b` to `__modsi3`. We reuse `__udivsi3`, which
 * returns the quotient in `d0` and the remainder in `d1` for the unsigned
 * operands we pass on the stack.
 *
 * Sign rules (C99): `a % b` has the same sign as `a` (toward zero).
 * This routine:
 * - Replaces divisor and dividend on the stack with their absolute values.
 * - Jumps to `__udivsi3` with a trampoline that may negate the remainder.
 *
 * Conceptual C:
 *
 *   int32_t __modsi3(int32_t a, int32_t b) {
 *     int sa = sign(a), sb = sign(b);
 *     uint32_t ua = uabs(a), ub = uabs(b);
 *     uint32_t q = __udivsi3(ua, ub); // ignored
 *     int32_t r = (int32_t)remainder_from_udiv; // in d1
 *     if (sa < 0) r = -r;
 *     return r;
 *   }
 *
 * Why assembly: same rationale as `__divsi3` — tight integration with the
 * unsigned core and custom return path (trampoline at `.Lret`).
 *
 * ABI note: `__udivsi3` must preserve `a0` (return address) and leave the
 * unsigned remainder in `d1`.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <asm.h>

ENTRY(__modsi3)
	/* NB: this requires that __udivsi3 preserve a0 and return */
	/* the modulus in d1: */
	move.l	(sp)+,a0	/* real return address — we jmp (a0) at the end */
	move.l	4(sp),d1	/* divisor */
	bpl.s	1f
	neg.l	4(sp)		/* store |divisor| for udiv */
1:	move.l	(sp),d0		/* dividend (stack layout after pop) */
	pea	.Lret(pc)	/* where `__udivsi3` will `rts` */
	bpl.s	2f
	neg.l	4(sp)		/* store |dividend| */
	/*
	 * `subq.l #2,(sp)` tweaks the stack so the udiv path sees the same layout
	 * as other callers (historical NetBSD trick — see original comments).
	 */
	subq.l	#2,(sp)
2:	jmp	_L(__udivsi3)
	neg.l	d1		/* apply sign of original dividend to remainder */
.Lret:	move.l	d1,d0		/* function result is in d0 */
	jmp	(a0)
END(__modsi3)

# vim: ft=gas:ts=8:sw=8:noet:
