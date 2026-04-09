/*
 * Miscellaneous runtime (libmisc): tables, compression hooks, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
/*
 * UAE-specific: trap into WinUAE/Fellow debug hooks via a known illegal opcode
 * sequence at a fixed address. On real hardware this path is skipped (rts).
 * Assembly is required: we must emit exact instruction words the emulator
 * intercepts; C cannot express fixed PC-relative illegal+jmp sequences.
 */
#include <config.h>
#include <asm.h>

ENTRY(_UaeTrap)
#ifdef UAE
	/* illegal + rts */
	cmp.l   #0xa0134e75,0xf0ff60
	beq     .Luae
	rts
.Luae:  jmp     0xf0ff60
#else
	rts
#endif
END(_UaeTrap)

STRONG_ALIAS(_UaeLog, _UaeTrap)
STRONG_ALIAS(_UaeWarpMode, _UaeTrap)

# vim: ft=gas:ts=8:sw=8:noet:
