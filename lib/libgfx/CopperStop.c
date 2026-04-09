/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <custom.h>
#include <copper.h>

/* Minimal valid copper list: single END instruction in CHIP (see CopListFinish). */
static __data_chip __aligned(2) u_int NullCopperList = 0xfffffffe;

/*
 * CopperStop — load empty lists on COP1/COP2, strobe COPJMP, disable copper/raster/sprite DMA.
 *
 * Why both cop1lc and cop2lc: AGA can chain lists; OCS mainly uses COP1. copjmp
 * forces immediate re-fetch. Disabling raster DMA stops bitplane DMA until re-enabled.
 */
void CopperStop(void) {
  /* Load empty list and instruct copper to jump to them immediately. */
  custom->cop2lc = (u_int)&NullCopperList;
  custom->copjmp2 = 0;
  custom->cop1lc = (u_int)&NullCopperList;
  custom->copjmp1 = 0;
  /* Disable copper DMA and all systems that rely on frame-by-frame refresh
   * by Copper. */
  DisableDMA(DMAF_COPPER | DMAF_RASTER | DMAF_SPRITE);
}
