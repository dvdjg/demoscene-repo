/*
 * TwisterRGB — “barrel” twister: per-line BPLMOD + chunky palette + border sprites.
 *
 * **Twist (hardware):** On each scanline the copper programs negative `bpl1mod` / `bpl2mod`
 * so bitplane DMA walks *backwards* within a very wide CHIP bitmap (`twister`). That
 * re-reads overlapping 16-pixel words and stretches/compresses the texture horizontally
 * in a cosine-shaped column (HRM: BPLxMOD stride between lines). `SetupLines` does not
 * rebuild the copper list: it patches the immediate words in the preallocated `bplmod`
 * table so each line’s modulus matches the delta of `rotate(f)` between consecutive
 * lines — the `(y1 - y0) - 1` term accounts for one less inter-line step than raw
 * vertical separation in the source map.
 *
 * **Colour (chunky RGB):** There are only 32 colour registers but five bitplanes → 32
 * pens. A separate pixmap (`texture_pixels`) holds true-colour-ish data: every third
 * scanline the list loads registers 1..31 from the texture (beam-synchronous “chunky”
 * look). Colour 0 is a simple vertical gradient (`gradient_colors`) for the background
 * strip between texture columns. `SetupTexture` scrolls the texture vertically by poking
 * the MOVE immediates in the active copper list (same idea as `bplmod` patches).
 *
 * **`bpldat[0]` per line:** Seeds plane 0’s serial path at line start so the first
 * fetched word after the modulo-controlled stream is in a known state (avoids garbage
 * at the left edge when mod wraps aggressively).
 *
 * **Sprites:** `left_*` / `right_*` on channels 4–7 frame the 144px-wide effect; fetch
 * is offset with `SetupBitplaneFetch(..., X(STARTX), WIDTH)` so the twisted strip sits
 * in the border layout.
 *
 * **Double copper:** `cp[0]`/`cp[1]` alternate — `CopListRun` after patching, VBlank,
 * then `active ^= 1` so the CPU never patches the list Denise is still using.
 *
 * HRM: https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 */
#include <effect.h>
#include <blitter.h>
#include <copper.h>
#include <fx.h>
#include <pixmap.h>
#include <sprite.h>
#include <system/memory.h>

#define WIDTH   144
#define HEIGHT  255
#define DEPTH   5
#define STARTX  96

static CopListT *cp[2];
static CopInsPairT *bplptr[2];
static CopInsT *bplmod[2][HEIGHT];
static CopInsT *colors[2][HEIGHT];
static short active = 0;

#include "data/twister-gradient.c"
#include "data/twister-texture.c"
#include "data/twister-left.c"
#include "data/twister-right.c"
#include "data/twister.c"

/* `n` is 0 or 1: duplicate structure so both lists can be patched independently in Render. */
static CopListT *MakeCopperList(short n) {
  CopListT *cp = NewCopList(100 + HEIGHT * 5 + (31 * HEIGHT / 3));
  CopInsPairT *sprptr = CopSetupSprites(cp);
  u_short *pixels = texture_pixels;
  short i, j, k;

  bplptr[n] = CopSetupBitplanes(cp, &twister, DEPTH);

  CopMove16(cp, dmacon, DMAF_SETCLR|DMAF_RASTER);
  CopSetColor(cp, 0, gradient_colors[0]);

  for (i = 0, k = 0; i < HEIGHT; i++) {
    CopWaitSafe(cp, Y(i), HP(0));
    /* Initial -32; real per-line twist values are written in `SetupLines`. */
    bplmod[n][i] = CopMove16(cp, bpl1mod, -32);
    CopMove16(cp, bpl2mod, -32);
    CopMove16(cp, bpldat[0], 0);

    CopSetColor(cp, 0, gradient_colors[i]);

    /* Chunky pass only on every third line to cap copper list size vs. update cost. */
    if ((i % 3) == 0) {
      colors[n][k++] = CopSetColor(cp, 1, *pixels++);
      for (j = 2; j < 32; j++)
        CopSetColor(cp, j, *pixels++);
    }
  }

  CopInsSetSprite(&sprptr[4], left_0);
  CopInsSetSprite(&sprptr[5], left_1);
  CopInsSetSprite(&sprptr[6], right_0);
  CopInsSetSprite(&sprptr[7], right_1);

  return CopListFinish(cp);
}

static void Init(void) {
  SetupBitplaneFetch(MODE_LORES, X(STARTX), WIDTH);
  SetupMode(MODE_LORES, DEPTH);
  SetupDisplayWindow(MODE_LORES, X(0), Y(0), 320, HEIGHT);

  cp[0] = MakeCopperList(0);
  cp[1] = MakeCopperList(1);

  SpriteUpdatePos(left_0, X(0), Y(0));
  SpriteUpdatePos(left_1, X(16), Y(0));
  SpriteUpdatePos(right_0, X(320 - 32), Y(0));
  SpriteUpdatePos(right_1, X(320 - 16), Y(0));

  /* Show `cp[1]` first so `Render` begins by patching the inactive list (`active == 0` → `cp[0]`). */
  CopListActivate(cp[1]);
  EnableDMA(DMAF_RASTER | DMAF_SPRITE);
}

static void Kill(void) {
  ResetSprites();
  CopperStop();

  DeleteCopList(cp[0]);
  DeleteCopList(cp[1]);
}

/* Vertical phase 0..255 into bitmap Y (cosine easing of the twist column). */
static inline short rotate(short f) {
  return (COS(f) >> 3) & 255;
}

static void SetupLines(short f) {
  short y0 = rotate(f);

  /* Absolute start row in `twister` for line 0 (all five plane pointers). */
  {
    int y = (short)twister_bytesPerRow * y0;
    void *const *planes = twister.planes;
    CopInsPairT *_bplptr = bplptr[active];
    short n = DEPTH;

    while (--n >= 0)
      CopInsSet32(_bplptr++, (*planes++) + y);
  }

  /* Per-line BPL1MOD/BPL2MOD: `m` is the inter-line rewind in bytes for this twist slope. */
  {
    short **modptr = (short **)bplmod[active];
    short y1, i, m;

    for (i = 1; i < HEIGHT; i++, y0 = y1, f += 2) {
      short *mod = *modptr++;

      y1 = rotate(f);
      m = ((y1 - y0) - 1) * (WIDTH / 8);

      /* mod[0/2] are instruction headers; [1] and [3] are the two MOVE.W immediates. */
      mod[1] = m;
      mod[3] = m;
    }
  }
}

/* Vertical scroll of the chunky strip: overwrite COLOR01..COLOR31 immediates in the list. */
static void SetupTexture(CopInsT **colors, short y) {
  u_short *pixels = texture_pixels;
  short n = texture_height;

  y = mod16(y, texture_height);
  pixels += y * texture_width;

  while (--n >= 0) {
    short *ins = (short *)*colors++;

    /* Stride 2: each copper MOVE is (opcode word, immediate word). */
    ins[1] = *pixels++;
    ins[3] = *pixels++;
    ins[5] = *pixels++;
    ins[7] = *pixels++;
    ins[9] = *pixels++;
    ins[11] = *pixels++;
    ins[13] = *pixels++;
    ins[15] = *pixels++;
    ins[17] = *pixels++;
    ins[19] = *pixels++;
    ins[21] = *pixels++;
    ins[23] = *pixels++;
    ins[25] = *pixels++;
    ins[27] = *pixels++;
    ins[29] = *pixels++;
    ins[31] = *pixels++;
    ins[33] = *pixels++;
    ins[35] = *pixels++;
    ins[37] = *pixels++;
    ins[39] = *pixels++;
    ins[41] = *pixels++;
    ins[43] = *pixels++;
    ins[45] = *pixels++;
    ins[47] = *pixels++;
    ins[49] = *pixels++;
    ins[51] = *pixels++;
    ins[53] = *pixels++;
    ins[55] = *pixels++;
    ins[57] = *pixels++;
    ins[59] = *pixels++;
    ins[61] = *pixels++;

    y++;

    if (y >= texture_height) {
      pixels = texture_pixels;
      y = 0;
    }
  }
}

PROFILE(TwisterRGB);

static void Render(void) {
  ProfilerStart(TwisterRGB);
  {
    SetupLines(frameCount * 16);
    SetupTexture(colors[active], frameCount);
  }
  ProfilerStop(TwisterRGB);

  CopListRun(cp[active]);
  TaskWaitVBlank();
  active ^= 1;
}

EFFECT(TwisterRGB, NULL, NULL, Init, Kill, Render, NULL);
