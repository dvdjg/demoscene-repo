/*
 * Blitter-oriented code (libblit): DMA-friendly operations.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <blitter.h>
#include <system/interrupt.h>

/*
 * BlitterStop — mask blitter interrupt, wait for idle, disable blitter DMA + blithog.
 *
 * BLITHOG: when set, blitter steals all cycles; clearing returns to fair share.
 * Order: disable IRQ to avoid spurious ISR, WaitBlitter until BLTDONE, then DMA off.
 */
void BlitterStop(void) {
  DisableINT(INTF_BLIT);
  WaitBlitter();
  DisableDMA(DMAF_BLITTER | DMAF_BLITHOG);
}
