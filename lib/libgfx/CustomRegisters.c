/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <custom.h>

/*
 * __bplcon3 / __fmode — shadow copies of write-only AGA registers (see custom.h).
 *
 * Copper MOVE helpers merge masks into these before writing to hardware so partial
 * updates do not clear other bits. OCS demos often leave defaults; AGA needs PF2OF, etc.
 */
u_short __bplcon3 = BPLCON3_PF2OF(3) | BPLCON3_SPRES(0);

u_short __fmode = 0;
