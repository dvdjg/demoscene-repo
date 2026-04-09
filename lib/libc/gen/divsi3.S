/*
 * __divsi3 — signed 32/32 -> 32 division for the GCC runtime (gen/divsi3.S).
 *
 * Purpose: the 68000 has `divs` / `divu` only for 16-bit divisors. GCC lowers
 * `a / b` where both are 32-bit integers to a call to `__divsi3`. This file
 * wraps the unsigned core `__udivsi3` after normalizing signs on the stack.
 *
 * Algorithm:
 * - Take absolute values of dividend and divisor *in place* on the stack so
 *   `__udivsi3` sees unsigned operands.
 * - XOR original signs (kept in d1/d0) to decide whether the unsigned quotient
 *   must be negated.
 *
 * Conceptual C:
 *
 *   int32_t __divsi3(int32_t num, int32_t den) {
 *     const int sign = (num < 0) ^ (den < 0);
 *     uint32_t u = (uint32_t)(num < 0 ? -num : num);
 *     uint32_t v = (uint32_t)(den < 0 ? -den : den);
 *     uint32_t q = __udivsi3(u, v);   // tail call in asm
 *     return sign ? -(int32_t)q : (int32_t)q;
 *   }
 *
 * Why assembly instead of C:
 * - This must be reentrant, minimal, and register-tuned; every cycle counts on
 *   a 7.14 MHz 68000 when division appears in inner loops.
 * - The tail call to `__udivsi3` avoids a second function prologue.
 *
 * Contract / ABI: `__udivsi3` must preserve `a0` because we use it to squirrel
 * away the real return address when we need to negate the quotient after the
 * unsigned divide returns.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
/* Based on divsi3.S from NetBSD */

#include <asm.h>

ENTRY(__divsi3)
	/* NB: this requires that __udivsi3 preserve a0 */
	move.l	4(sp),d1	/* dividend — keep original sign in d1 for later */
	bpl.s	1f
	neg.l	4(sp)		/* replace stack slot with |dividend| */
1:	move.l	8(sp),d0	/* divisor */
	bpl.s	2f
	neg.l	8(sp)		/* replace stack slot with |divisor| */
2:	eor.l	d1,d0		/* d0 = sign(divisor) XOR sign(dividend) */
	bpl.s	3f		/* if signs matched, quotient is nonnegative */
	/*
	 * Signs differ: unsigned quotient must be negated. We cannot negate after
	 * `__udivsi3` returns with a plain `rts` because that returns to the
	 * caller of `__divsi3`. Pop the return addr into a0, push a trampoline
	 * that negates d0, then jump to `__udivsi3` which will `rts` to .Lret.
	 */
	move.l	(sp)+,a0
	pea	.Lret(pc)
3:	jmp	_L(__udivsi3)
.Lret:	neg.l	d0		/* apply sign to unsigned quotient */
	jmp	(a0)		/* return to original caller */
END(__divsi3)

STRONG_ALIAS(ldivs,__divsi3)

# vim: ft=gas:ts=8:sw=8:noet:
