/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
/*
 * CopSetupMode — append copper MOVEs for BPLCON0/FMODE (resolution, depth, HAM, etc.).
 * Implementation: SetupModeImpl.c (shared with CPU SetupMode path).
 */
#include <copper.h>

#include "SetupModeImpl.c"
