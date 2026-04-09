/*
 * __mulsi3 — signed 32x32 -> 32 multiply for GCC runtime (gen/mulsi3.S).
 *
 * Purpose: the 68000 has `muls`/`mulu` that produce a 32-bit product only from
 * 16-bit x 16-bit operands. A full 32-bit multiply is expanded by the compiler
 * into this helper.
 *
 * Mathematical decomposition (all unsigned half-products):
 *   x = (x1<<16) | x0,  y = (y1<<16) | y0
 *   x*y = x*y1<<16 + x*y0
 *       = x1*y1<<32 + x0*y1<<16 + x1*y0<<16 + x0*y0
 * Here we accumulate the 32-bit result (discarding overflow above 32 bits for
 * signed-mul semantics as implemented by this classic sequence).
 *
 * Conceptual C (unsigned 16-bit chunks):
 *
 *   uint32_t mul32(uint32_t x, uint32_t y) {
 *     uint16_t x0 = x, x1 = x >> 16, y0 = y, y1 = y >> 16;
 *     uint32_t acc = 0;
 *     acc += (uint32_t)(x0 * y1) << 16;
 *     acc += (uint32_t)(x1 * y0) << 16;
 *     acc += (uint32_t)(x1 * y1) << 0; // high half — see swap/clr in asm
 *     return acc;
 *   }
 *
 * Why assembly: four `mulu` + shifts + adds in the minimum number of cycles;
 * scheduling is hand-tuned for 68000.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <asm.h>

ENTRY(__mulsi3)
        move.w  4(sp),d0	/* x0 -> d0 */
        mulu.w  10(sp),d0	/* x0 * y1 (cross term) */
        move.w  6(sp),d1	/* x1 -> d1 */
        mulu.w  8(sp),d1	/* x1 * y0 (cross term) */
        add.w   d1,d0
        swap    d0
        clr.w   d0		/* position cross terms into high word */
        move.w  6(sp),d1	/* x1 */
        mulu.w  10(sp),d1	/* x1 * y1 (high product) */
        add.l   d1,d0
        rts
END(__mulsi3)

# vim: ft=gas:ts=8:sw=8:noet:
