/*
 * Low-level Amiga autovector interrupt dispatch (levels 1–6).
 *
 * Purpose: hardware delivers IRQs to 68000 autovectors; we map INTB_* bits in
 * INTREQR (masked by INTENAR) to per-IRQ handlers in IntVec. EnterIntr clears
 * the pending bit in INTREQ and calls the C handler with user data.
 *
 * Why assembly: IRQ entry must be cycle-tight, preserve exactly the registers
 * the ABI expects, and touch custom.intreq — that is textbook hand-written
 * interrupt glue on Amiga.
 *
 * MULTITASK: optional reschedule path when returning from IPL0 to task context.
 *
 * HRM (interrupts, INTREQ/INTENA): https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 */
#include <asm.h>
#include <config.h>
#include <custom_regdef.h>

.macro  FIRST   intb
        /* Prologue for each autovector level:
         * - save scratch regs used by dispatcher,
         * - read pending interrupts (INTREQR & INTENAR),
         * - test first candidate bit and branch to EnterIntr if set.
         *
         * Conceptual C:
         *   save(d0,d1,a0,a1);
         *   d1 = intb;
         *   d0 = custom->intreqr & custom->intenar;
         *   if (d0 & (1 << d1)) goto EnterIntr;
         */
        movem.l d0-d1/a0-a1,-(sp)
        moveq   #\intb,d1
        move.w  custom+intreqr,d0
        and.w   custom+intenar,d0
        btst    d1,d0
        jne     _L(EnterIntr)
.endm

.macro  NEXT    irq
        /* Scan next candidate bit on same interrupt level. */
        addq.w  #1,d1
        btst    d1,d0
        jne     _L(EnterIntr)
.endm

.macro  FINISH
        /* Nothing matched: restore and return. */
        jra     _L(LeaveIntr)
.endm

# Level 1 Interrupt Autovector

ENTRY(AmigaLvl1Handler)
        FIRST   INTB_TBE
        NEXT    INTB_DSKBLK
        NEXT    INTB_SOFTINT
        FINISH
END(AmigaLvl1Handler)

# Level 2 Interrupt Autovector

ENTRY(AmigaLvl2Handler)
        FIRST   INTB_PORTS
        FINISH
END(AmigaLvl2Handler)

# Level 3 Interrupt Autovector

ENTRY(AmigaLvl3Handler)
        FIRST   INTB_COPER
        NEXT    INTB_VERTB
        NEXT    INTB_BLIT
        FINISH
END(AmigaLvl3Handler)

# Level 6 Interrupt Autovector

ENTRY(AmigaLvl6Handler)
        FIRST   INTB_EXTER
        FINISH
END(AmigaLvl6Handler)

# Main part of interrupt handler

ENTRY(EnterIntr)
        /*
         * d1 = INTB_* bit index selected by FIRST/NEXT.
         *
         * Conceptual C:
         *   uint16_t mask = 1u << d1;
         *   custom->intreq = mask;      // acknowledge/clear source
         *   IntVecEntry *iv = &IntVec[d1];
         *   iv->code(iv->data);
         */
        /* Make INTF_* mask, and clear pending interrupt. */
        clr.w   d0
        bset    d1,d0
        move.w  d0,custom+intreq

        lea     _L(IntVec),a0
        lsl.w   #3,d1
        add.w   d1,a0

        /* Enter interrupt service routine. */
        move.l  (a0)+,a1                /* IntVecEntryT.code */
        move.l  (a0)+,a0                /* IntVecEntryT.data */
        jsr     (a1)
END(EnterIntr)

ENTRY(LeaveIntr)
#ifdef MULTITASK
        /*
         * Check if we need to reschedule a task - usually as a result waking
         * up a higher priorty task while running interrupt service routine.
         * Do it only when returning from interrupt context to task context.
         * Let's not do the switch while returning from nested interrupt.
         */
        move.w  16(sp),d0               /* SR from interrupted context */
        and.w   #0x0700,d0              /* was IPL set to 0 (task context) ? */
        jne     .Lnoswitch

        /* Turn off interrupts to prevent data races on NeedReschedule. */
        or.w    #0x0700,sr

        lea     _L(NeedReschedule),a0
        tst.b   (a0)                    /* check reschedule flag */
        jeq     .Lnoswitch

        clr.b   (a0)                    /* clear reschedule flag */
        movem.l (sp)+,d0-d1/a0-a1       /* restore dispatcher registers */
        /*
         * Jump into YieldHandler after its initial SR mask instruction (+4):
         * we already forced IPL=7 above, so re-executing `or.w #0x0700,sr`
         * is redundant.
         */
        jra     _L(YieldHandler)+4      /* force a task switch */
#endif

.Lnoswitch:
        movem.l (sp)+,d0-d1/a0-a1
        rte
END(LeaveIntr)

# Level 4 Interrupt Autovector

ENTRY(AmigaLvl4Handler)
        FIRST   INTB_AUD0
        NEXT    INTB_AUD1
        NEXT    INTB_AUD2
        NEXT    INTB_AUD3
        FINISH
END(AmigaLvl4Handler)

# Level 5 Interrupt Autovector

ENTRY(AmigaLvl5Handler)
        FIRST   INTB_RBF
        NEXT    INTB_DSKSYNC
        FINISH
END(AmigaLvl5Handler)

# Dummy handler

ENTRY(DummyInterruptHandler)
        rts
END(DummyInterruptHandler)

.macro  SAVECTX
        /* assumes SR and PC are already pushed on stack */
        /*
         * Save full task context in layout expected by TaskSwitch():
         *   [USP][D0..A6][...hardware-pushed SR/PC...]
         * Equivalent to a "software context frame".
         */
        or.w    #0x0700,sr              /* mask all interrupts */
        movem.l d0-a6,-(sp)             /* save D0-A6 registers */
        move.l  usp,a0                  /* save USP register */
        move.l  a0,-(sp)
        move.l  _L(CurrentTask),a0      /* load current task pointer */
        move.l  sp,(a0)                 /* save stack pointer */
.endm

.macro  LOADCTX
        /* Reverse SAVECTX and return with RTE (restores SR+PC atomically). */
        move.l  _L(CurrentTask),a0      /* load current task pointer */
        move.l  (a0),sp                 /* restore stack pointer */
        move.l  (sp)+,a0                /* restore USP register */
        move.l  a0,usp
        movem.l (sp)+,d0-a6             /* restore D0-A6 registers */
        rte                             /* restore SR and PC */
.endm

#ifdef MULTITASK
ENTRY(YieldHandler)
        SAVECTX
        jsr     _L(TaskSwitch)
        LOADCTX
END(YieldHandler)
#endif

ENTRY(CrashHandler)
        SAVECTX
        jmp     _L(Crash)
END(CrashHandler)

# vim: ft=gas:ts=8:sw=8:noet:
