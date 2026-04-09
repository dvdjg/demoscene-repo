/*
 * setjmp — save a minimal call context for non-local goto (gen/setjmp.S).
 *
 * Purpose: ISO C `setjmp`/`longjmp` for this environment. The `jmp_buf` layout
 * (see include/setjmp.h) stores callee-saved registers d2-d7, a2-a7 (note a7 is
 * the stack pointer), and the saved PC at offset `_JB_PC * 4`.
 *
 * What we save:
 * - All registers that the SysV/m68k ABI expects the callee to preserve across
 *   calls (`d2-d7`, `a2-a7`). d0/d1/a0/a1 are caller-saved and therefore not
 *   part of the snapshot — they are *not* restored by `longjmp`.
 * - The return address of `setjmp` itself (top of stack) so `longjmp` can fake a
 *   return to the caller of `setjmp`.
 *
 * Return value: `setjmp` returns 0 when called directly; `longjmp` supplies a
 * nonzero value by patching state elsewhere (see longjmp.S).
 *
 * Conceptual C (illustrative only):
 *
 *   typedef struct jmp_buf {
 *     uint32_t regs[12]; // d2-d7,a2-a7
 *     void *pc;
 *   } jmp_buf;
 *
 *   int setjmp(jmp_buf *jb) {
 *     jb->regs = save_d2_d7_a2_a7();
 *     jb->pc = __builtin_return_address(0);
 *     return 0;
 *   }
 *
 * Why assembly: `movem` saves 12 registers in one instruction; a C compiler
 * would emit a large prologue/epilogue.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <asm.h>

/*
 * ABI: a0 points to jmp_buf (see `setjmp(jmp_buf asm("a0"))` in setjmp.h).
 */
ENTRY(setjmp)
        movem.l d2-d7/a2-a7,(a0)        /* snapshot callee-saved registers */
        move.l  (sp),48(a0)             /* return address of this setjmp call */
        moveq.l #0,d0                   /* direct call returns 0 */
        rts
END(setjmp)

# vim: ft=gas:ts=8:sw=8:noet:
