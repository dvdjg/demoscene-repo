/*
 * Blitter-oriented code (libblit): DMA-friendly operations.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <blitter.h>

/*
 * Deferred blit state — BlitterCopySetup computes all registers except pointers
 * and BLTSIZE; BlitterCopyStart supplies plane pointers and kicks BLTSIZE.
 * Why split: same geometry for multiple planes (copy each bitplane separately).
 */
typedef struct {
  const BitmapT *src;
  const BitmapT *dst;
  u_int start;   /* Byte offset in dst plane to top-left of blit rectangle */
  u_short size;  /* BLTSIZE: height in bits 15–6, width in words in bits 5–0 */
} StateT;

static StateT state[1];

/*
 * BlitterCopySetup — configure blitter for a copy from full src bitmap to dst at (x,y).
 * Supports arbitrary x (sub-word alignment) and any src width (may exceed one row).
 *
 * Path A (bltshift != 0): use B+C channels with shift in BLTCON1 — A is implicit;
 *   minterm (SRCB|SRCC|DEST)|(ABC|NABC|ABNC|NANBC) = standard masked merge.
 * Path B (bltshift == 0): simple A→D copy (SRCA|DEST|A_TO_D), no B/C.
 *
 * bltafwm/bltalwm: first/last word masks from FirstWordMask/LastWordMask for partial words.
 * Modulos: difference between stride and active width so DMA advances correctly at EOL.
 */
void BlitterCopySetup(const BitmapT *dst, u_short x, u_short y,
                      const BitmapT *src)
{
  /* Calculate real blit width. It can be greater than src->bytesPerRow! */
  u_short width = (x & 15) + src->width;
  u_short bytesPerRow = ((width + 15) & ~15) >> 3;
  u_short srcmod = src->bytesPerRow - bytesPerRow;
  u_short dstmod = dst->bytesPerRow - bytesPerRow;
  u_short bltafwm = FirstWordMask[x & 15];
  u_short bltalwm = LastWordMask[width & 15];
  u_short bltshift = rorw(x & 15, 4);

  state->src = src;
  state->dst = dst;
  state->start = ((x & ~15) >> 3) + y * dst->bytesPerRow;
  state->size = (src->height << 6) | (bytesPerRow >> 1);

  WaitBlitter();

  if (bltshift) {
    custom->bltcon0 = (SRCB | SRCC | DEST) | (ABC | NABC | ABNC | NANBC);
    custom->bltbmod = srcmod;
    custom->bltadat = -1;
    custom->bltcmod = dstmod;
  } else {
    custom->bltamod = 0;
    custom->bltcon0 = (SRCA | DEST) | A_TO_D;
  }

  custom->bltcon1 = bltshift;
  custom->bltafwm = bltafwm;
  custom->bltalwm = bltalwm;
  custom->bltdmod = dstmod;
}

/*
 * BlitterCopyStart — set pointers for given plane indices and write BLTSIZE (starts DMA).
 * Waits for previous blit to finish before reprogramming pointers.
 */
void BlitterCopyStart(short dstbpl, short srcbpl) {
  void *srcbpt = state->src->planes[srcbpl];
  void *dstbpt = state->dst->planes[dstbpl] + state->start;
  u_short bltsize = state->size;

  WaitBlitter();

  custom->bltapt = srcbpt;
  custom->bltbpt = srcbpt;
  custom->bltcpt = dstbpt;
  custom->bltdpt = dstbpt;
  custom->bltsize = bltsize;
}
