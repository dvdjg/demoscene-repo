/*
 * Playfield setup — BPLCON0/1/2/3, DIW, DDF, FMODE for display resolution and depth.
 *
 * MODE_* bits OR into BPLCON0/FMODE; must match chipset (OCS vs AGA). Dual playfield
 * (MODE_DUALPF) uses even/odd planes as two independent 16-color layers.
 * Setup* functions write custom registers once; copper can override per-line later.
 * HRM "Playfield Hardware": https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 */
#ifndef __PLAYFIELD_H__
#define __PLAYFIELD_H__

#include <beampos.h>

#define MODE_LORES 0
#define MODE_HIRES __BIT(15)     /* BPLCON0_HIRES — 640 horizontal pixels */
#define MODE_DUALPF __BIT(10)    /* BPLCON0_DBLPF — two playfields + priorities in BPLCON2 */
#define MODE_LACE __BIT(2)       /* BPLCON0_LACE — interlaced */
#define MODE_HAM __BIT(11)       /* BPLCON0_HOMOD — hold-and-modify (6 bits) */
#define MODE_SHRES __BIT(6)            /* AGA BPLCON0_SHRES */
#define MODE_16BIT 0
#define MODE_32BIT __BIT(0)            /* FMODE_BLP32 */
#define MODE_64BIT (__BIT(0)|__BIT(1)) /* FMODE_BPAGEM,FMODE_BLP32 */
#define MODE_LINEDBL __BIT(14)         /* FMODE_BSCAN2 */

void SetupBitplaneFetch(u_short mode, hpos xstart, u_short width);

/* DIW: display window in low-res coordinates (xstart, ystart, width, height). */
void SetupDisplayWindow(u_short mode, hpos xstart, vpos ystart,
                        u_short width, u_short height);

void SetupMode(u_short mode, u_short depth);

/* Convenience: fetch + window + mode in one call. */
void SetupPlayfield(u_short mode, u_short depth,
                    hpos xstart, vpos ystart, u_short width, u_short height);

#endif
