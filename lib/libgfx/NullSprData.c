/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <sprite.h>

/*
 * NullSprData — minimal valid sprite struct in CHIP (pos/ctl zero, no pixel rows).
 * Point SPRxPT here to hide a channel without undefined DMA.
 */
__data_chip SpriteT NullSprData[] = {{ .pos = 0, .ctl = 0, .data = {} }};
