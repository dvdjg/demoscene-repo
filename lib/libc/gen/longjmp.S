/*
 * longjmp — restore a context saved by setjmp (gen/longjmp.S).
 *
 * Purpose: jump back to the dynamic extent of `setjmp` as if `setjmp` were
 * returning again with a caller-provided value.
 *
 * ABI (see include/setjmp.h):
 *   void longjmp(jmp_buf asm("a0"), int asm("d1"));
 * The second argument (`val`) is passed in d1 per the prototype.
 *
 * What this routine does:
 * - `movem` reloads registers from the jmp_buf at `a0`, post-incrementing `a0`
 *   past the saved register block.
 * - The word at the next address is the saved PC; we overwrite the stack slot
 *   that `rts` will pop so execution resumes as if returning from `setjmp`.
 *
 * Conceptual C:
 *
 *   __noreturn void longjmp(jmp_buf *jb, int val) {
 *     restore_d2_d7_a2_a7(jb->regs);
 *     // NOTE: C standard says if val==0, setjmp must appear to return 1.
 *     // This implementation may not adjust that — see NOTE below.
 *     __builtin_return_address_override(jb->pc);
 *     // unreachable
 *   }
 *
 * Why assembly: same as setjmp — `movem` is the compact way to restore many
 * registers; we also need precise control of the stack return slot.
 *
 * NOTE: possible issue: the header places the return value in d1, but the C
 * `longjmp` result is observed in d0 by the caller of `setjmp`. If the compiler
 * does not emit a move from d1 to d0 at the call site, the return value may be
 * wrong; also ISO C requires `longjmp(..., 0)` to behave as if `1` were passed.
 * Verify generated code at call sites for `longjmp` / `setjmp` in this build.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <asm.h>

/*
 * ABI: a0 = jmp_buf, d1 = int value to return from setjmp (per setjmp.h).
 */
ENTRY(longjmp)
#if defined(__ELF__)
        move.l 4(%sp),%a0
        move.l 8(%sp),%d1
#endif
        movem.l (a0)+,d2-d7/a2-a7       /* restore callee-saved regs incl. SP */
        move.l  (a0),(sp)               /* patch return address for upcoming rts */
        rts
END(longjmp)

# vim: ft=gas:ts=8:sw=8:noet:
