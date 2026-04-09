/*
 * UserState34 — leave supervisor mode on Kickstart 1.3 (ExecV33/34).
 *
 * Purpose: Exec's UserState is broken on early KS for 68010+; this custom
 * sequence restores USP and SR safely. See amigaos.c which calls this instead of
 * UserState when ExecVer <= 34.
 *
 * What this routine does (68000 programmer’s model):
 * - The CPU has two stack pointers: SSP (supervisor stack pointer) when the S
 *   bit in SR is set, and USP (user stack pointer) when running at user level.
 * - To return from supervisor to user mode you must: load the user stack into
 *   USP, point SP at the user stack, then clear the supervisor bit in SR.
 *
 * Register usage at entry (per call site in amigaos.c):
 * - a0: desired user stack pointer value (typically the USP saved on entry to
 *   supervisor mode, or another valid user stack).
 *
 * Conceptual C (not real API — illustrates semantics):
 *
 *   void UserState34(void *user_sp) {
 *     void (*ret)(void) = pop_return_address();
 *     __asm__ volatile("move.l %%sp,%%usp" : : : "memory");
 *     __asm__ volatile("move.l %0,%%sp" : : "a"(user_sp) : "memory");
 *     __asm__ volatile("and.w #0xdfff,%%sr" ::: "memory"); // leave supervisor
 *     ret();
 *   }
 *
 * The immediate `0xdfff` masks out bit 13 of SR on the 68000 (0x2000), which is
 * the supervisor (S) flag: `(sr & ~0x2000)` forces user mode. Trace/other bits
 * outside the low word are not affected by this `and.w` on the 68000 (SR is 16
 * bits); on later CPUs SR is wider — this sequence is chosen to match the
 * historical Exec workaround.
 *
 * Why assembly: you must manipulate SR, SP, and USP in one atomic narrative;
 * a C compiler cannot emit the exact supervisor/user transition required here
 * without non-portable extensions, and the call/return pattern must preserve the
 * return address in a1.
 *
 * Hardware / OS docs:
 *   HRM: https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 *   RKM (Exec): https://archive.org/details/amiga-rom-kernel-reference-manual
 */
#include <asm.h>

ENTRY(UserState34)
        /* Pop return address into a1 — we will jmp (a1) after switching stacks. */
        movea.l (sp)+,a1
        /* Current supervisor SP becomes the user stack pointer value for later. */
        move.l  sp,usp
        /* Install the caller-provided user stack from a0 as the active SP. */
        movea.l a0,sp
        /* Clear supervisor bit in SR — execution continues in user mode. */
        and.w   #0xdfff,sr
        /* Tail-return: jump to the caller’s return address (not rts on this SP). */
        jmp     (a1)
END(UserState34)

# vim: ft=gas:ts=8:sw=8:noet:
