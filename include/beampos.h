/*
 * Raster beam position — read VPOSR and copper WAIT coordinates (PAL).
 *
 * VP/HP are in "copper coordinates": not the same as DIW pixel coords; see HRM
 * "Copper Hardware" for WAIT ranges. Polling VPOSR burns CPU; prefer VBlank IRQ
 * for sync when possible.
 * HRM: https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 */
#ifndef __BEAMPOS_H__
#define __BEAMPOS_H__

#include <custom.h>

/* Horizontal position for CopWait (low-res pixel-ish units; hardware-dependent). */
typedef struct hpos {
  short hpos;
} hpos;

/* Wrap hpos in struct so CopWait macros get type-checked. C equivalent: plain short. */
#define HP(x) ((hpos){.hpos = (x)})

/* Vertical line counter value for waits (PAL ~0..311; bit 8 wraps in copper). */
typedef struct vpos {
  short vpos;
} vpos;

#define VP(y) ((vpos){.vpos = (y)})

/*
 * DIWHP/DIWVP — default start of display window in register units (PAL).
 * X()/Y() convert user (0,0)=top-left of window to beam coordinates for copper.
 */
#define DIWHP 0x81
#define DIWVP 0x2c

typedef struct DispWin {
  hpos left;
  vpos top;
  hpos right;
  vpos bottom;
} DispWinT;

#define X(x) HP((x) + DIWHP)
#define Y(y) VP((y) + DIWVP)

/* Rightmost HP the copper can WAIT on reliably (chip limit; see copper.h notes). */
#define LASTHP HP(0xDE << 1)

/*
 * WaitLine — busy-wait until VPOSR line field matches (raster at given line).
 * C equivalent: `while ((custom->vposr >> 8) & 0x1ff != line)` with correct mask.
 * Why used: quick sync without IRQ; avoid in tight multitask if possible.
 */
static inline void WaitLine(vpos vp) {
  uint32_t line = vp.vpos;
  while ((custom->vposr_ & 0x1ff00) != ((line << 8) & 0x1ff00));
}

/* PAL: line ~303 is past active display — common "vblank" wait for CLI code. */
static inline void WaitVBlank(void) { WaitLine(VP(303)); }

#endif /* !__BEAMPOS_H__ */
