/*
 * SetIPL — set 68000 interrupt priority level (SR bits 10–8).
 *
 * Purpose: mask or unmask IRQ levels atomically; used when entering/exiting
 * critical sections that must not nest badly with interrupt handlers.
 *
 * Why assembly: manipulating SR must be done with precise instruction
 * ordering; a tiny leaf routine avoids compiler surprises and matches the
 * usual M68k idiom (read SR, mask, merge new IPL, write SR).
 */
#include <asm.h>

ENTRY(SetIPL)
        /*
         * Conceptual C equivalent:
         *   uint16_t old = get_sr();
         *   uint16_t newsr = (old & ~0x0700) | (new_ipl & 0x0700);
         *   set_sr(newsr);
         *   return old & 0x0700;
         *
         * Using asm is mandatory here because SR access is privileged and must
         * stay as a tiny, strictly ordered critical-section primitive.
         */
        move.w  sr,-(sp)                /* save SR for return value */
        move.w  sr,d1
        and.w   #0xf8ff,d1              /* old SR with IPL bits cleared */
        and.w   #0x0700,d0              /* argument: new IPL in bits 10–8 */
        or.w    d0,d1
        move.w  d1,sr                   /* apply new IPL (critical section) */
        move.w  (sp)+,d0                /* pop saved SR into d0 */
        and.w   #0x0700,d0              /* return old IPL only (mask) */
        rts
END(SetIPL)

# vim: ft=gas:ts=8:sw=8:noet
