/*
 * Weave — “woven” playfield + sprite layers with Copper-timed priority swaps.
 *
 * What you see (demoscene technique):
 * - A wide horizontal colour bar (`bar.c`, 480 px) scrolls sinusoidally behind a
 *   288 px window. Only part of the bar is visible; sub-pixel smooth scroll uses
 *   BPLCON1 (PF1/PF2 nibble shuffle) and word-aligned pointer bumps, same family
 *   of tricks as horizontal fine scroll on OCS.
 * - The bar image is taller than one scanline: the Copper repeats the *same*
 *   raster line many times using negative BPLxMOD (“hold” the fetch pointer),
 *   then briefly switches to a positive modulo (`bar_bplmod`) to advance one row
 *   in CHIP, then holds again. That is how a tall gradient is painted down the
 *   screen without a 1:1 framebuffer — classic “line multiply” / copper
 *   vertical scaling (see HRM: BPL1MOD/BPL2MOD, fetch sequencing).
 * - Five vertical “columns” of raster time (O0..O4) run left→right. On each line
 *   the Copper waits to those horizontal positions and toggles BPLCON2 so PF2
 *   (the bar) is sometimes above the sprite stripes and sometimes below them.
 *   That per-column priority flip is what produces the weave: alternating bands
 *   where opaque sprite pixels cover the bar or the bar covers the sprites.
 * - Sprite data (`stripes.c`) is a pre-baked vertical ramp; animating `sprpt`
 *   with different phases (`fu` vs `fd`) scrolls the ramp up on some sprites and
 *   down on others for motion inside the mask.
 *
 * Why hardware instead of CPU:
 * - Priority between playfields and sprites is a few bits in BPLCON2; changing
 *   them at exact horizontal positions is what the Copper is for. Doing this on
 *   the CPU would require impossible cycle timing per column.
 * - Line repeat via modulo is essentially free once the list is built; a CPU
 *   fill would touch every pixel.
 *
 * References:
 *   HRM — Copper, BPLxMOD, BPLCON1, BPLCON2, sprite control:
 *         https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 *   HRM mirror: http://amigadev.elowar.com/read/
 */
#include "effect.h"

#include "custom.h"
#include "copper.h"
#include "blitter.h"
#include "bitmap.h"
#include "sprite.h"
#include "fx.h"

#include "data/bar.c"
#include "data/bar-2.c"
#include "data/colors.c"
#include "data/stripes.c"

/*
 * bar_bplmod — bytes to add to the bitplane pointers after each displayed line
 * when we *advance* one row in the bar bitmap (see my==16 in MakeCopperList).
 * Derived from (bar_width - WIDTH) so the fetch walks the correct distance in
 * planar bytes for a bar graphic wider than the visible window (bar_width 480).
 */
#define bar_bplmod ((bar_width - WIDTH) / 8 - 2)

/* Visible playfield: 288×256 @ 4 bitplanes (16 colours for the bar area). */
#define WIDTH (320 - 32)
#define HEIGHT 256
#define DEPTH 4
#define NSPRITES 8

/*
 * O0..O4 — horizontal Copper WAIT positions (HP units) for the five columns.
 * They are expressed as offsets from DIWHP (see beampos.h) so they track the
 * display window. The list interleaves: WAIT → patchable sprite moves → BPLCON2.
 */
#define O0 (DIWHP + 0)
#define O1 (DIWHP + 56)
#define O2 (DIWHP + 112)
#define O3 (DIWHP + 172)
#define O4 (DIWHP + 224)

/*
 * Per-buffer bookkeeping for everything the CPU must rewrite each frame.
 * - sprite: first SPRxPTR pair moves (chunky stripe texture pointers).
 * - bar: initial BPLxPT + BPLCON1 shuffle for the scrolling bar.
 * - bar_change[4]: four Copper sites where we reset scroll/modulo at the start
 *   of each vertical “stripe” of the bar schedule (lines with my==48).
 * - stripes[HEIGHT]: first CopIns of the per-line horizontal sprite position block
 *   for column 0 (other columns patched at +4,+8,… offsets in UpdateStripeState).
 */
typedef struct State {
  CopInsPairT *sprite;
  CopInsPairT *bar;
  CopInsT *bar_change[4];
  CopInsT *stripes[HEIGHT];
} StateT;

/* Double-buffered copper state: active^1 is built while the other list is live. */
static int active = 1;
static CopListT *cp[2];
/* Downsampled sine (0..127) for fast horizontal wobble of sprite columns. */
static short sintab8[128];
static StateT state[2];

#define STRIPES 5
#define BARS 4

/*
 * StripePhase* — independent animation phase per column; must stay odd because
 * UpdateStripeState advances phase by 2 in the inner asm (addqb #2) so the low
 * bit never toggles; odd starting phases preserve that invariant for table walks.
 */
static u_char StripePhase[STRIPES] = { 4, 24, 16, 8, 12 };
static char StripePhaseIncr[STRIPES] = { 8, -10, 14, -6, 6 };

/*
 * Emit Copper MOVEs that (re)initialize both sprite position words for a pair of
 * sprite indices to 0 before UpdateStripeState overwrites them with real HP.
 * spr[n*2+0/1] maps CopIns offsets for sprite control words (see custom.h).
 */
static inline void CopSpriteSetHP(CopListT *cp, short n) {
  CopMove16(cp, spr[n * 2 + 0].pos, 0);
  CopMove16(cp, spr[n * 2 + 1].pos, 0);
}

/* Upper bound on MOVE/WAIT count: one tall per-line section + head/tail slack. */
#define COPLIST_SIZE (HEIGHT * 22 + 100)

/*
 * Build a static Copper list with two kinds of tricks:
 * (1) Vertical structuring of the bar via modulo and colour reloads.
 * (2) Horizontal “weave” via timed BPLCON2 priority changes and sprite HP.
 */
static CopListT *MakeCopperList(StateT *state) {
  CopListT *cp = NewCopList(COPLIST_SIZE);
  short b, y;

  /* Initial bar fetch: four BPL pointers + horizontal scroll nibble state. */
  state->bar = CopMove32(cp, bplpt[0], NULL);
  CopMove32(cp, bplpt[1], NULL);
  CopMove32(cp, bplpt[2], NULL);
  CopMove32(cp, bplpt[3], NULL);
  CopMove16(cp, bplcon1, 0);

  /*
   * Negative modulo pulls the pointer back after each line so the same row of
   * the bar bitmap is refetched — vertical stretch / hold-line. The magnitude
   * matches one line’s worth of planar bytes plus DMA housekeeping (-2 term).
   */
  CopMove16(cp, bpl1mod, -WIDTH / 8 - 2);
  CopMove16(cp, bpl2mod, -WIDTH / 8 - 2);

  /* First block of SPRxPT: default pointers (overwritten in Render). */
  CopMove32(cp, sprpt[0], stripes_0); /* up */
  CopMove32(cp, sprpt[1], stripes_1);
  CopMove32(cp, sprpt[2], stripes_2); /* down */
  CopMove32(cp, sprpt[3], stripes_3);
  CopMove32(cp, sprpt[4], stripes_0); /* up */
  CopMove32(cp, sprpt[5], stripes_1);
  CopMove32(cp, sprpt[6], stripes_2); /* down */
  CopMove32(cp, sprpt[7], stripes_3);

  CopWait(cp, Y(-1), HP(0));

  /*
   * Second SPRxPT block: point at raw sprite data (SprDataT) for DMA; UpdateSpriteState
   * scrolls animation by offsetting these pointers within the CHIP arrays.
   */
  state->sprite = CopMove32(cp, sprpt[0], stripes_0->data); /* up */
  CopMove32(cp, sprpt[1], stripes_1->data);
  CopMove32(cp, sprpt[2], stripes_2->data); /* down */
  CopMove32(cp, sprpt[3], stripes_3->data);
  CopMove32(cp, sprpt[4], stripes_0->data); /* up */
  CopMove32(cp, sprpt[5], stripes_1->data);
  CopMove32(cp, sprpt[6], stripes_2->data); /* down */
  CopMove32(cp, sprpt[7], stripes_3->data);

  for (y = 0, b = 0; y < HEIGHT; y++) {
    vpos vp = Y(y);
    short my = y & 63;

    CopWaitSafe(cp, vp, HP(0));

    /*
     * Per-line vertical state machine inside each 64-line band (my = y % 64):
     * - my==8: swap 16-colour palette between bar / bar-2 for a two-tone cycle.
     * - my==16: one line of “real” vertical advance in the bar bitmap.
     * - my==48: capture a patch point; reset scroll + modulos for the next bar
     *   segment (CPU fills real values in UpdateBarState).
     * - my==49: re-arm the hold-line negative modulo for the stretched section.
     * At most three Copper MOVEs (bplcon1 / bpl1mod / bpl2mod) on lines that hit
     * these branches — keeps DMA safe within one scanline.
     */
    if (my == 8) {
      if (y & 64) {
        CopLoadColors(cp, bar_colors, 0);
      } else {
        CopLoadColors(cp, bar2_colors, 0);
      }
    } else if (my == 16) {
      CopMove16(cp, bpl1mod, bar_bplmod);
      CopMove16(cp, bpl2mod, bar_bplmod);
    } else if (my == 48) {
      state->bar_change[b++] = cp->curr;
      CopMove16(cp, bplcon1, 0);
      CopMove16(cp, bpl1mod, 0);
      CopMove16(cp, bpl2mod, 0);
    } else if (my == 49) {
      CopMove16(cp, bpl1mod, -WIDTH / 8 - 2);
      CopMove16(cp, bpl2mod, -WIDTH / 8 - 2);
    }

    {
      /*
       * BPLCON2 PF2Px fields decide where playfield 2 sits relative to sprites.
       * Toggling between PF2P_SP07 and PF2P_BOTTOM swaps whether PF2 draws over
       * or under the sprite layer in a horizontal band — the “weave”.
       * p0/p1 alternate per 64-line band so the pattern inverts half-way down.
       */
      short p0, p1;

      if (y & 64) {
        p0 = BPLCON2_PF2P_SP07, p1 = BPLCON2_PF2P_BOTTOM;
      } else {
        p0 = BPLCON2_PF2P_BOTTOM, p1 = BPLCON2_PF2P_SP07;
      }

      /*
       * Five columns: wait → record patch ptr for sprite HP → set sprite pair
       * positions (placeholder 0) → apply priority. Sprite indices 0–3 are the
       * vertical masks; index 0 is reused at O4 to close the column pattern.
       * Small +4 on each HP nudges the WAIT past copper/DIW startup skew so the
       * BPLCON2 write lands in stable raster time for that column (tuned for PAL).
       */
      CopWait(cp, vp, HP(O0 + 4));
      state->stripes[y] = cp->curr;
      CopSpriteSetHP(cp, 0);
      CopMove16(cp, bplcon2, p0);
      CopWait(cp, vp, HP(O1 + 4));
      CopSpriteSetHP(cp, 1);
      CopMove16(cp, bplcon2, p1);
      CopWait(cp, vp, HP(O2 + 4));
      CopSpriteSetHP(cp, 2);
      CopMove16(cp, bplcon2, p0);
      CopWait(cp, vp, HP(O3 + 4));
      CopSpriteSetHP(cp, 3);
      CopMove16(cp, bplcon2, p1);
      CopWait(cp, vp, HP(O4 + 4));
      CopSpriteSetHP(cp, 0);
      CopMove16(cp, bplcon2, p0);
    }
  }

  return CopListFinish(cp);
}

/*
 * Scroll the wide bar horizontally: word-aligned pointer bump + BPLCON1 nibble
 * scroll for the fractional pixel. `bar_change[]` patches the Copper moves at
 * my==48 so each vertical segment stays consistent when the sine phase steps.
 */
static void UpdateBarState(StateT *state) {
  short w = (bar_width - WIDTH) / 2;
  short f = frameCount * 16;
  short bx = w + normfx(SIN(f) * w);

  {
    CopInsPairT *ins = state->bar;

    /* Even word offset into the bar bitmap; shift = fine 0..15 horizontal phase. */
    short offset = (bx >> 3) & -2;
    short shift = ~bx & 15;

    CopInsSet32(&ins[0], bar.planes[0] + offset);
    CopInsSet32(&ins[1], bar.planes[1] + offset);
    CopInsSet32(&ins[2], bar.planes[2] + offset);
    CopInsSet32(&ins[3], bar.planes[3] + offset);
    /* BPLCON1: same nibble in low and high nybble scrolls PF1/PF2 together. */
    CopInsSet16((CopInsT *)&ins[4], (shift << 4) | shift);
  }

  {
    CopInsT **insp = state->bar_change;
    short shift, offset, bplmod, bx_prev, i;

    /*
     * Four bar segments (one per my==48 line): each uses a slightly advanced
     * sine phase so the horizontal scroll continues smoothly down the frame.
     * `offset` captures 16-pixel steps; bplmod adjusts planar stride when the
     * coarse position crosses a 16px boundary.
     */
    for (i = 0; i < BARS; i++) {
      CopInsT *ins = *insp++;

      f += SIN_HALF_PI;
      bx_prev = bx;
      bx = w + normfx(SIN(f) * w);

      shift = ~bx & 15;
      offset = (bx & -16) - (bx_prev & -16);
      bplmod = bar_bplmod + (offset >> 3);

      CopInsSet16(&ins[0], (shift << 4) | shift);
      CopInsSet16(&ins[1], bplmod);
      CopInsSet16(&ins[2], bplmod);
    }
  }
}

/*
 * Animate vertical ramps in the sprite shapes by offsetting DMA pointers into
 * the pre-expanded stripe tables. `fu` advances forward, `fd` backward — paired
 * sprites move opposite directions for counter-posed motion.
 */
static void UpdateSpriteState(StateT *state) {
  CopInsPairT *ins = state->sprite;
  int fu = frameCount & 63;
  int fd = (~frameCount) & 63;

  CopInsSet32(ins++, stripes_0->data + fu); /* up */
  CopInsSet32(ins++, stripes_1->data + fu);
  CopInsSet32(ins++, stripes_2->data + fd); /* down */
  CopInsSet32(ins++, stripes_3->data + fd);
  CopInsSet32(ins++, stripes_0->data + fu); /* up */
  CopInsSet32(ins++, stripes_1->data + fu);
  CopInsSet32(ins++, stripes_2->data + fd); /* down */
  CopInsSet32(ins++, stripes_3->data + fd);
}

/* Map a Copper Ox HP constant into sprite horizontal position units (+8 offset). */
#define HPOFF(x) ((x + 32) / 2)

/*
 * Update every sprite horizontal position used in the weave columns.
 *
 * Each of the five columns uses a phase index into sintab8; the phase walks by
 * the per-column increment each frame. Per scanline, horizontal position is
 * sintab8[phase] + column_base_offset; the second word is offset +8 (pairing).
 *
 * The loop processes 8 lines per inner iteration (unrolled BODY) × 32 outer
 * iterations = 256 lines — matches HEIGHT.
 *
 * Inline asm (conceptual C for one BODY):
 *
 *   hp = sintab8[(u_char)phase];
 *   phase = (u_char)(phase + 2);   // addqb #2
 *   hp += hp_off;
 *
 * Why asm: keeps `phase` in a data register with `addqb` (byte add with extend)
 * and uses a single indexed load — fewer instructions than GCC would emit for
 * the hot triple-nested loop. A plain C version would be correct but slower;
 * this is the innermost per-frame cost.
 */
static void UpdateStripeState(StateT *state) {
  static const char offset[STRIPES] = {
    HPOFF(O0), HPOFF(O1), HPOFF(O2), HPOFF(O3), HPOFF(O4) };
  u_char *phasep = StripePhase;
  char *incrp = StripePhaseIncr;
  const char *offsetp = offset;
  short i;

  for (i = 0; i < STRIPES * 4; i += 4) {
    u_int phase = (u_char)*phasep;
    CopInsT **stripesp = state->stripes;
    short hp_off = *offsetp++;
    short n = HEIGHT / 8 - 1;

    (*phasep++) += (*incrp++);

    do {
      CopInsT *ins;
      short hp;

#define BODY()                                          \
      ins = *stripesp++;                                \
      asm ("movew (%2,%1:w),%0\n\t"                     \
           "addqb #2,%1\n\t"                            \
           "addw  %3,%0"                                \
           : "=d" (hp), "+d" (phase)                    \
           : "a" (sintab8), "d" (hp_off));              \
      CopInsSet16(&ins[i + 0], hp);                     \
      CopInsSet16(&ins[i + 1], hp + 8);

      BODY(); BODY(); BODY(); BODY();
      BODY(); BODY(); BODY(); BODY();
    } while (--n != -1);
  }
}

/* Downsample global 512-entry sine to 128 bytes for cheap sprite wobble. */
static void MakeSinTab8(void) {
  int i, j;

  for (i = 0, j = 0; i < 128; i++, j += 32)
    sintab8[i] = (sintab[j] + 512) >> 10;
}

static void Init(void) {
  MakeSinTab8();

  SetupDisplayWindow(MODE_LORES, X(16), Y(0), WIDTH, HEIGHT);
  SetupBitplaneFetch(MODE_LORES, X(0), WIDTH + 16);
  SetupMode(MODE_LORES, DEPTH);
  LoadColors(bar_colors, 0);
  LoadColors(stripes_colors, 16);

  /*
   * Base BPLCON2: PF2 has priority over PF1; PF2 sits between sprite pair bands
   * (SP2–7) so the copper can still flip PF2 vs SP0–1 per column. Init sets
   * sprite Y/attach via SpriteUpdatePos; horizontal motion is all Copper+CPU.
   */
  custom->bplcon2 = BPLCON2_PF2PRI | BPLCON2_PF2P_SP27 | BPLCON2_PF1P_SP27;

  SpriteUpdatePos(stripes_0, X(0), Y(0));
  SpriteUpdatePos(stripes_1, X(0), Y(0));
  SpriteUpdatePos(stripes_2, X(0), Y(0));
  SpriteUpdatePos(stripes_3, X(0), Y(0));

  cp[0] = MakeCopperList(&state[0]);
  cp[1] = MakeCopperList(&state[1]);
  CopListActivate(cp[0]);

  EnableDMA(DMAF_RASTER|DMAF_SPRITE);
}

static void Kill(void) {
  DisableDMA(DMAF_RASTER|DMAF_SPRITE);
  DeleteCopList(cp[0]);
  DeleteCopList(cp[1]);
}

PROFILE(UpdateStripeState);

/*
 * Frame pipeline: bar scroll + sprite texture phase + heavy stripe HP update,
 * then swap the copper list built for this buffer and wait for vertical blank.
 */
static void Render(void) {
  UpdateBarState(&state[active]);
  UpdateSpriteState(&state[active]);

  ProfilerStart(UpdateStripeState);
  UpdateStripeState(&state[active]);
  ProfilerStop(UpdateStripeState);

  CopListRun(cp[active]);
  WaitVBlank();
  active ^= 1;
}

EFFECT(Weave, NULL, NULL, Init, Kill, Render, NULL);
