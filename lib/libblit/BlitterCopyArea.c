/*
 * Blitter-oriented code (libblit): DMA-friendly operations.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <blitter.h>

/*
 * Deferred blit state for sub-rectangle copy (Area2D) with optional reverse DMA.
 * fast: word-aligned width and no shift — simple A→D path; else B+C merge with shift.
 */
typedef struct {
  const BitmapT *src;
  const BitmapT *dst;
  u_int src_start;
  u_int dst_start;
  u_short size;
  bool fast;
} StateT;

static StateT state[1];

#define sx area->x
#define sy area->y
#define sw area->w
#define sh area->h

#define START(x) (((x) & ~15) >> 3)
#define ALIGN(x) START((x) + 15)

/* This routine assumes following conditions:
 *  - there's always enough space in `dst` to copy area from `src`
 *  - at least one of `sx` and `dx` is aligned to word boundary 
 */
/*
 * BlitterCopyAreaSetup — blit `area` from src to dst at (dx,dy).
 *
 * forward: blit direction when shift alignment differs (avoid overlap corruption).
 * BLITREVERSE on backward path. fast path skips B channel (straight copy).
 */
void BlitterCopyAreaSetup(const BitmapT *dst, u_short dx, u_short dy,
                          const BitmapT *src, const Area2D *area)
{
  /* Destination and source fine scroll (pixel offset inside first word). */
  u_short dxo = dx & 15;
  u_short sxo = sx & 15;
  /* Forward when destination starts at same/later bit position than source. */
  bool forward = dxo >= sxo;
  u_short xo = forward ? dxo : sxo;
  u_short width = xo + sw;
  u_short wo = width & 15;
  u_short bytesPerRow = ALIGN(width);
  u_short srcmod = src->bytesPerRow - bytesPerRow;
  u_short dstmod = dst->bytesPerRow - bytesPerRow;
  u_short bltafwm = FirstWordMask[dxo];
  u_short bltalwm = LastWordMask[wo];
  u_short bltshift = rorw(xo, 4);

  /*
   * TODO: Two cases exist where number of word for 'src' and 'dst' differ.
   * 1. (sxo < (sxo + area->w) & 15) && (dxo > (dxo + area->w) & 15)
   * 2. (sxo > (sxo + area->w) & 15) && (dxo < (dxo + area->w) & 15)
   * To handle them an in-memory mask must be created as suggested by HRM:
   * http://amigadev.elowar.com/read/ADCD_2.1/Hardware_Manual_guide/node0121.html
   */

  state->src = src;
  state->dst = dst;
  state->src_start = START(sx);
  state->dst_start = START(dx);
  state->size = (sh << 6) | (bytesPerRow >> 1);

  if (forward) {
    state->src_start += (short)sy * (short)src->bytesPerRow;
    state->dst_start += (short)dy * (short)dst->bytesPerRow;
  } else {
    /* Reverse mode starts from last word of last row to avoid overwrite hazards. */
    state->src_start += (short)(sy + sh - 1) * (short)src->bytesPerRow
                      + bytesPerRow - 2;
    state->dst_start += (short)(dy + sh - 1) * (short)dst->bytesPerRow
                      + bytesPerRow - 2;
  }

  state->fast = (xo == 0) && (wo == 0);

  WaitBlitter();

  if (!state->fast) {
    custom->bltcon0 = (SRCB | SRCC | DEST) | (ABC | NABC | ABNC | NANBC);
    custom->bltcon1 = bltshift | (forward ? 0 : BLITREVERSE);
    custom->bltadat = -1;
    if (forward) {
      custom->bltafwm = bltafwm;
      custom->bltalwm = bltalwm;
    } else {
      custom->bltafwm = bltalwm;
      custom->bltalwm = bltafwm;
    }
    custom->bltbmod = srcmod;
    custom->bltcmod = dstmod;
    custom->bltdmod = dstmod;
  } else {
    custom->bltcon0 = (SRCA | DEST) | A_TO_D;
    custom->bltcon1 = 0;
    custom->bltafwm = -1;
    custom->bltalwm = -1;
    custom->bltamod = srcmod;
    custom->bltdmod = dstmod;
  }
}

void BlitterCopyAreaStart(short dstbpl, short srcbpl) {
  /* Plane pointers plus cached offsets from Setup. */
  void *srcbpt = state->src->planes[srcbpl] + state->src_start;
  void *dstbpt = state->dst->planes[dstbpl] + state->dst_start;
  u_short bltsize = state->size;

  if (state->fast) {
    WaitBlitter();

    custom->bltapt = srcbpt;
    custom->bltdpt = dstbpt;
    custom->bltsize = bltsize;
  } else {
    WaitBlitter();

    custom->bltbpt = srcbpt;
    custom->bltcpt = dstbpt;
    custom->bltdpt = dstbpt;
    custom->bltsize = bltsize;
  }
}
