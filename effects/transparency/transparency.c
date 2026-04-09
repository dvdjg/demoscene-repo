/*
 * Transparency — fake translucency without alpha (OCS/ECS).
 *
 * What you see:
 * - A two-bitplane “Ghostown” logo moves over a parallax stardust field. There is
 *   no true alpha channel: every visible colour is one of 16 entries chosen by the
 *   four bitplane bits at each pixel (2 planes stardust + 2 planes logo buffer).
 *
 * Core demoscene trick — palette compositing:
 * - Real transparency would blend RGB values; the Amiga has no framebuffer blend unit.
 *   Instead, `ChangePalette` assigns each of the 16 possible 4-bit indices a colour
 *   that *looks* like a blend between background palette (`background_colors`) and
 *   logo palette (`logo_colors`), weighted by `s` via `ColorTransition`.
 * - Where logo bits are 0, you see pure stardust colours; where they are 1, you see
 *   interpolated mixes — the eye reads it as see-through paint.
 *   (This is not EHB / half-brite, which is a 6th plane trick on AGA; the comment
 *   “EHB-style” here only means “extra shades from clever colour mapping”.)
 *
 * Second trick — copper vertical split + scroll:
 * - Stardust bitmaps are 448×192, 2 planes (see `stardust_1_width` / `_height` in data).
 *   The display window uses a 320px-wide fetch; `BPLxMOD` is set so DMA walks the
 *   wider CHIP buffer correctly (stride − displayed width in bytes).
 * - A `CopWait` at a programmable line (`midpoint`) switches `bpl1mod` to a
 *   different value below the split: the lower screen region then repeats / offsets
 *   fetches differently, so the tall stardust layer does not cover the whole frame
 *   the same way — classic raster “window” into a larger texture.
 * - `CopInsSet16(bplshift, ~xo & 15)` updates BPLCON1 for horizontal fine scroll
 *   (PF1/PF2 nibble shuffle; see HRM “Scroll” and BPLCON1).
 * - `midpoint->wait.vp` moves the split line with `yo` so the visible stardust band
 *   slides vertically while the logo bounces independently.
 *
 * Third trick — blitter stamp:
 * - Each frame the logo planes are copied into `screen` with `BitplaneCopyFast`:
 *   SRCA→D with `ASH` shift in `BLTCON0` for arbitrary pixel alignment (`x & 15`).
 *   That is the standard way to composite aligned artwork into planar CHIP without
 *   a CPU inner loop (HRM “Blitter”, minterms, shifts).
 *
 * Division of labour:
 * - Copper: beam-synchronized pointers, modulo, fine scroll, split line.
 * - Blitter: 2-D stamp of logo into off-screen buffer.
 * - CPU: patch a handful of `CopIns` values + full palette (`ChangePalette`) per frame.
 *
 * HRM (Blitter, Copper, Denise colour): https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * HRM mirror: http://amigadev.elowar.com/read/
 */
#include "effect.h"
#include "copper.h"
#include "color.h"
#include "blitter.h"
#include "fx.h"

/* Logo destination: low-res 320×256, 2 bitplanes (4 logo colours indexed in palette). */
#define WIDTH   320
#define HEIGHT  256
#define DEPTH   2

/* CHIP bitmap receiving the logo blits each frame (cleared at allocation). */
static __code BitmapT *screen;
/*
 * Patched Copper MOVE handles (no full list rebuild per frame):
 * - bplptr: four `CopInsPairT` slots for BPL1PT..BPL4PT — stardust planes 0/1 and
 *   screen planes 0/1 interleaved as bplpt[0..3].
 * - bplshift: BPLCON1 fine horizontal scroll for the layered fetch.
 * - midpoint: the WAIT before the second `bpl1mod` — vertical position of the split.
 */
static __code CopInsPairT *bplptr;
static __code CopInsT *bplshift;
static __code CopInsT *midpoint;
static __code CopListT *cp;

#include "data/ghostown-logo.c"
#include "data/stardust-1.c"
#include "data/stardust-2.c"
#include "data/stardust-3.c"
#include "data/stardust-4.c"
#include "data/stardust-5.c"
#include "data/stardust-6.c"

/* Six parallax frames; `Render` picks `f = mod16(frameCount>>1, 6)`. */
static const BitmapT *stardust[6] = {
  &stardust_1,
  &stardust_2,
  &stardust_3,
  &stardust_4,
  &stardust_5,
  &stardust_6,
};

/*
 * Blit one logo plane into the destination at pixel (x,y).
 *
 * - `dstbpt` targets the word containing the leftmost 16px column that covers the
 *   sprite (`(x & ~15) >> 3` byte offset) plus `y * bytesPerRow`.
 * - `bltshift = rorw(x & 15, 4)` encodes the A-channel funnel shift in BLTCON0
 *   bits so pixels slide to the correct fine-X position (HRM: ASH, “area mode”).
 * - When shift ≠ 0, the blit is one word wider (`bltsize` +1 width word), last-mask
 *   (`bltalwm`) becomes 0, and A modulo is −2 so the extra source word is consumed
 *   correctly — standard shifted-copy recipe.
 *
 * Conceptual C for the unshifted case: a nested loop OR’ing src into dst; the blitter
 * does it in DMA time with one `bltsize` kick.
 */
static void BitplaneCopyFast(BitmapT *dst, short d, u_short x, u_short y,
                             const BitmapT *src, short s)
{
  void *srcbpt = src->planes[s];
  void *dstbpt = dst->planes[d] + ((x & ~15) >> 3) + y * dst->bytesPerRow;
  u_short dstmod = dst->bytesPerRow - src->bytesPerRow;
  u_short bltsize = (src->height << 6) | (src->bytesPerRow >> 1);
  u_short bltshift = rorw(x & 15, 4);

  WaitBlitter();

  if (bltshift) {
    bltsize += 1; dstmod -= 2;

    custom->bltalwm = 0;
    custom->bltamod = -2;
  } else {
    custom->bltalwm = -1;
    custom->bltamod = 0;
  }

  custom->bltdmod = dstmod;
  custom->bltcon0 = (SRCA | DEST | A_TO_D) | bltshift;
  custom->bltcon1 = 0;
  custom->bltafwm = -1;
  custom->bltapt = srcbpt;
  custom->bltdpt = dstbpt;
  custom->bltsize = bltsize;
}

/*
 * Rebuild the 16 colour registers for the 4-plane display.
 *
 * Bitplane bits (from low to high) can be read as: bg low, bg high, fg low, fg high
 * (see comments column “C / F / B” — colour indices in the asset palettes). Pure
 * background combos (no logo bits) use `background_colors` unchanged; any combo
 * with foreground bits uses `ColorTransition` toward the matching `logo_colors[]`.
 *
 * `s` controls how strong the “ghost” is: low → closer to stars, high → closer to
 * solid logo ink. Driven by a sine in `Render` so the blend pulses over time.
 *
 * Why not do this in the blitter: the hardware cannot alpha-blend; one write to
 * COLORxx per index is cheap and gives smooth global animation.
 */
static void ChangePalette(short s) {
  short b0 = background_colors[0];
  short b1 = background_colors[1];
  short b2 = background_colors[2];
  short b3 = background_colors[3];
  short f1 = logo_colors[1];
  short f2 = logo_colors[2];
  short f3 = logo_colors[3];

  /* Index = bits[3:0] = (fg_hi fg_lo bg_hi bg_lo); comments decode palette slots. */
  /* 0000 — bg only */
  SetColor(0, b0);
  /* 0001 */
  SetColor(1, b1);
  /* 0010 — fg bit 0 on bg 0 */
  SetColor(2, ColorTransition(b0, f1, s));
  /* 0011 */
  SetColor(3, ColorTransition(b1, f1, s));
  /* 0100 */
  SetColor(4, b2);
  /* 0101 */
  SetColor(5, b3);
  /* 0110 */
  SetColor(6, ColorTransition(b2, f1, s));
  /* 0111 */
  SetColor(7, ColorTransition(b3, f1, s));
  /* 1000 — fg bit 1 */
  SetColor(8, ColorTransition(b0, f2, s));
  /* 1001 */
  SetColor(9, ColorTransition(b1, f2, s));
  /* 1010 */
  SetColor(10, ColorTransition(b0, f3, s));
  /* 1011 */
  SetColor(11, ColorTransition(b1, f3, s));
  /* 1100 */
  SetColor(12, ColorTransition(b2, f2, s));
  /* 1101 */
  SetColor(13, ColorTransition(b3, f2, s));
  /* 1110 */
  SetColor(14, ColorTransition(b2, f3, s));
  /* 1111 — both fg bits set */
  SetColor(15, ColorTransition(b3, f3, s));
}

/*
 * Static copper list: initial BPL pointers, modulo for wide stardust over 320px
 * fetch, optional fine scroll, then a mid-frame WAIT that reloads `bpl1mod` so the
 * raster below the split uses a negative modulo.
 *
 * Why two modulos: above the split, positive `bpl1mod = stride − displayWidth` lets
 * Agnus walk a 448px-wide asset while only 320px are shown (each line skips the
 * right-hand padding in CHIP). Below the split, a large negative modulo pulls the
 * pointer back after each line so the same source row can be re-fetched — that
 * “holds” a line of stardust while the raster still advances, giving a different
 * vertical band without a second full bitmap. (Exact banding depends on stride math;
 * see HRM BPL1MOD/BPL2MOD and fetch sequencing.)
 *
 * `midpoint` captures the WAIT node so `Render` can move `vp` and slide the split
 * up/down (`DIWVP + stardust_1_height - 1 - yo`).
 */
static CopListT *MakeCopperList(void) {
  CopListT *cp = NewCopList(30);

  bplptr = CopInsPtr(cp);
  CopMove32(cp, bplpt[0], stardust_1.planes[0]);
  CopMove32(cp, bplpt[1], screen->planes[0]);
  CopMove32(cp, bplpt[2], stardust_1.planes[1]);
  CopMove32(cp, bplpt[3], screen->planes[1]);
  /* stardust_1_bytesPerRow − (320/8): skip padding after each 320px line in CHIP. */
  CopMove16(cp, bpl1mod, stardust_1_bytesPerRow - 320 / 8);
  CopMove16(cp, bpl2mod, 0);

  bplshift = CopInsPtr(cp);
  CopMove16(cp, bplcon1, 0);

  midpoint = CopInsPtr(cp);
  CopWait(cp, Y(stardust_1_height - 1), HP(0));
  /* Below split: strong negative modulo pulls pointers back relative to upper half. */
  CopMove16(cp, bpl1mod, -stardust_1_bytesPerRow - 320 / 8);
  CopListFinish(cp);

  return cp;
}

/*
 * Allocate logo buffer, enable 4 bitplanes (2 stardust + 2 logo), crop left border
 * by 16px in the DIW so the effect lines up visually, build copper, turn on DMA.
 */
static void Init(void) {
  screen = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR);

  SetupMode(MODE_LORES, stardust_1_depth + DEPTH);
  SetupBitplaneFetch(MODE_LORES, X(0), WIDTH);
  SetupDisplayWindow(MODE_LORES, X(16), Y(0), WIDTH-16, HEIGHT);

  ChangePalette(8);
  cp = MakeCopperList();

  CopListActivate(cp);
  EnableDMA(DMAF_RASTER | DMAF_BLITTER);
}

static void Kill(void) {
  CopperStop();
  DisableDMA(DMAF_RASTER | DMAF_BLITTER);
  DeleteBitmap(screen);
  DeleteCopList(cp);
}

/*
 * Per-frame pipeline:
 * 1) Blit logo planes with Lissajous-style motion (`SIN` on x/y at different rates).
 * 2) Animate palette blend `s` — changes all 16 COLOR registers (see ChangePalette).
 * 3) Pick stardust frame `f`, compute large-amplitude scroll `xo`/`yo`, patch:
 *    - BPLCON1 fine scroll (`~xo & 15` — same convention as other effects using
 *      inverted low nibble for shuffle direction),
 *    - BPLxPT for both stardust planes with a byte-linear offset into CHIP,
 *    - WAIT vertical position so the copper split follows `yo` (parallax band moves).
 *
 * `TaskWaitVBlank` at end: stay in lockstep with the beam after editing copper.
 */
static void Render(void) {
  {
    short xo = normfx(SIN(frameCount * 8) * 32);
    short yo = normfx(SIN(frameCount * 16) * 32);

    BitplaneCopyFast(screen, 0, 80 + xo, 64 + yo, &logo, 0);
    BitplaneCopyFast(screen, 1, 80 + xo, 64 + yo, &logo, 1);
  }
 
  {
    short s = normfx(SIN(frameCount * 64) * 6) + 8;
    ChangePalette(s);
  }

  {
    short f = mod16(frameCount >> 1, 6);
    short xo = normfx(SIN(frameCount * 32) * 64) + 64;
    short yo = normfx(COS(frameCount * 32) * 64) + 64;
    /*
     * Word-aligned byte offset into the stardust bitmap: coarse X scroll in CHIP
     * plus full-line steps for Y. Fine sub-word motion is done with BPLCON1, not
     * by bumping pointers (avoids unaligned DMA and matches 16px blit alignment).
     */
    int offset = ((xo >> 3) & -2) + yo * stardust_1_bytesPerRow;

    CopInsSet16(bplshift, ~xo & 15);
    CopInsSet32(&bplptr[0], stardust[f]->planes[0] + offset);
    CopInsSet32(&bplptr[2], stardust[f]->planes[1] + offset);
    midpoint->wait.vp = DIWVP + stardust_1_height - 1 - yo;
  }

  TaskWaitVBlank();
}

EFFECT(Transparency, NULL, NULL, Init, Kill, Render, NULL);
