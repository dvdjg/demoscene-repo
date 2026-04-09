/*
 * Graphics helper (libgfx): bitmaps, lines, copper helpers, etc.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
/*
 * Thin wrapper: real BPLCON0/FMODE writes live in SetupModeImpl.c (included).
 */
#include <playfield.h>

#include "SetupModeImpl.c"
