/*
 * __udivsi3 — unsigned 32/32 -> 32 division for the GCC runtime (gen/udivsi3.S).
 *
 * Purpose: implement `uint32_t / uint32_t` (and the core of signed division)
 * on 68000-class CPUs that only have `divu.w` / `divs.w` (16-bit divisor).
 *
 * High-level idea (NetBSD algorithm):
 * - Scale both operands down by the same power of two until the divisor fits
 *   in 16 bits (so `divu.w` is legal and cannot overflow its 32-bit result).
 * - Use a two-part `divu` on the high and low halves of the dividend to build
 *   a quotient *estimate* that is never too small; it may be one too large.
 * - Multiply back by the original divisor and subtract from the original
 *   dividend; if the remainder is negative, subtract one from the quotient and
 *   fix the remainder.
 *
 * Why assembly: this is hot-path code for any 32-bit division; a C version
 * would compile to many loads/stores and obscure the careful register reuse.
 * Also `divu`/`mulu` pairing is idiomatic for this CPU.
 *
 * Register contract for callers:
 * - `__divsi3` / `__modsi3` rely on `a0` being preserved across this routine
 *   (see their tail calls and trampoline returns).
 * - The remainder is left in `d1` on exit — `__umodsi3` depends on that.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
/* Based on divsi3.S from NetBSD */

#include <asm.h>

ENTRY(__udivsi3)
	move.l	d2,-(sp)	/* callee-saved temp for multiply/subtract phase */
	move.l	12(sp),d0	/* divisor */
	move.l	8(sp),d1	/* dividend */

/*
 * first, we divide the divisor and dividend by two until 
 * the divisor fits into 16 bits:
 */
1:	cmp.l	#0x10000,d0
	bcs.s	2f
	lsr.l	#1,d0
	lsr.l	#1,d1
	bra.s	1b
2:

/*
 * now we can do the divide.  to avoid overflow, we have to 
 * do the divide in two parts, high and low, and add the 
 * results together:
 */
	/* Conceptual C for the next block: split dividend into 16-bit halves */
	move.w	d1,d2		/* save low(dividend) */
	clr.w	d1
	swap	d1		/* d1 = dividend >> 16 */
	divu.w	d0,d1		/* high half / divisor — quotient in upper word */
	move.l	d1,a1		/* stash high-half result */
	move.w	d2,d1		/* rebuild 32-bit value for remainder/low split */
	divu.w	d0,d1		/* low-stage divide */
	move.l	a1,d0		/* recover high divide result */
	swap	d0
	clr.w	d0		/* d0 = high quotient contribution */
	and.l	#0xffff,d1	/* mask to low quotient bits */
	add.l	d1,d0		/* d0 = quotient guess (may be 1 too high) */

/*
 * the quotient we have so far is only a guess.  the divide we 
 * did above was really the divide of some dividendB by some 
 * divisorB, where the following hold:
 *
 * (dividend - divisor) <= dividendB <= dividend
 * (divisor / 2) < divisorB <= divisor
 *
 * so our guess quotient cannot be less than our real desired
 * quotient.  however, it might be one too big.
 *
 * to adjust this quotient, we multiply it by the original 
 * divisor and subtract the result from the original dividend.  
 * if the result is nonnegative, our guessed quotient was 
 * correct, and the subtraction result is our remainder.  
 * if the result is negative, our guessed quotient was one 
 * too big, and the subtraction result plus the original 
 * divisor is our remainder.
 *
 * as in mulsi3, we have to do the multiply in stages to avoid 
 * overflow:
 */

	/* 32x32 multiply using several 16x16 `mulu` — avoids overflow in steps */
	move.l	12(sp), d2	/* original (unscaled) divisor */
	swap	d2
	move.l	d0,d1
	mulu.w	d2,d1		/* high(divisor) * low(guess) */
	move.l	d1,a1		/* partial product */
	swap	d2
	move.l	d0,d1
	swap	d1
	mulu.w	d2,d1		/* low(divisor) * high(guess) */
	add.l	a1,d1
	swap	d1
	clr.w	d1		/* align high product of 32x32 */
	move.l	d2,a1		/* a1 = original divisor */
	mulu.w	d0,d2		/* low * low */
	add.l	d1,d2		/* d2 = guess * divisor (full 32-bit product) */
	
	move.l	8(sp),d1	/* original dividend */
	sub.l	d2,d1		/* remainder = dividend - guess*divisor */
	bcc.s	3f
	subq.l	#1,d0		/* quotient was too large: subtract 1 */
	add.l	a1,d1		/* adjust remainder by +divisor */
3:	move.l	(sp)+,d2	/* restore callee-saved */
	rts
END(__udivsi3)

STRONG_ALIAS(ldivu,__udivsi3)

# vim: ft=gas:ts=8:sw=8:noet:
