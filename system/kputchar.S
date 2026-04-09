/*
 * KPutChar — serial/debug output via the serial UART (custom.serdat / serdatr).
 *
 * Purpose: minimal polled output for very early debug or tiny monitors; waits
 * for TBE (transmit buffer empty) then writes one byte. Converts LF to CR+LF
 * like a console.
 *
 * Amiga hardware notes (OCS/ECS):
 * - `serdatr` is the serial data + status read register; bit 5 (often documented
 *   as TBE / transmit buffer state) is polled here so we do not overrun the
 *   UART: we only write `serdat` when the transmitter can accept a new word.
 * - `serdat` transmit data is written as a 16-bit word: the low byte is the
 *   character; the high byte sets framing (here `0x100` adds a stop bit in the
 *   position this codebase expects — see HRM chapter on the serial port).
 *
 * Conceptual C equivalent:
 *
 *   void KPutChar(struct Custom *cust, char c) {
 *     if (c == 0)
 *       return;
 *     if (c == '\n')
 *       KPutChar(cust, '\r');           // recursive in C; asm inlines via bsr
 *     uint16_t w = (uint8_t)c | 0x100; // stop bit / format bits in high byte
 *     while (!(cust->serdatr & (1u << 5)))
 *       ;                               // busy-wait for TBE
 *     cust->serdat = w;
 *   }
 *
 * Why assembly instead of C:
 * - Callable from almost any context (no libc, no stack frame assumptions).
 * - Predictable instruction count for “last gasp” debugging right after a crash.
 * - Direct register access without reloading `custom` from a global each time
 *   when the caller already has `a0` pointing at `Custom`.
 *
 * HRM (serial port, Paula): https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 */
#include <asm.h>
#include <custom_regdef.h>

/*
 * ABI for this routine:
 *   a0.l -> pointer to `struct Custom` (chip registers base)
 *   d0.b -> character to send (zero is ignored — useful for ASCIIZ iteration)
 */
ENTRY(KPutChar)
        /* C strings often end with 0; sending a NUL would confuse terminals. */
        tst.b   d0
        beq.s   .Quit

        /* Preserve the original character while we optionally emit CR first. */
        move.w  d0,-(sp)
        cmp.b   #10,d0          /* '\n' -> prefix '\r' for canonical TTY behavior */
        bne.s   .NoNL
        moveq   #13,d0
        bsr.s   .Send
.NoNL:  move.w  (sp)+,d0

.Send:  move.w  #0x100,d1       /* OR in stop-bit / framing pattern (see HRM) */
        move.b  d0,d1           /* low byte is the 8-bit character */

.Wait:  btst.b  #5,serdatr(a0)  /* wait until transmitter can accept data */
        beq.s   .Wait
        move.w  d1,serdat(a0)   /* strobe character into the UART */

.Quit:  rts
END(KPutChar)

# vim: ft=gas:ts=8:sw=8:noet:
