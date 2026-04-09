/*
 * Stripes — 3D-ish “floating bars” drawn as per-raster colour stripes (Copper).
 *
 * Demoscene technique:
 * - There are no polygon edges on screen — only horizontal spans. Each “stripe” is
 *   a stack of scanlines that share a vertical position `y` (after projection) and
 *   a thickness derived from `z` (pseudo-depth). `SetLineColor` writes the same
 *   COLOR0 into a run of `CopSetColor` instructions, one per display line, so the
 *   Copper changes the border/playfield colour exactly at each horizontal retrace
 *   boundary (classic “raster bar” / “copper rainbow” family of effects).
 * - `RotateStripes` projects random 3D seed positions with a 2D rotation around an
 *   off-screen pivot (`centerY`, `centerZ`), then perspective divide (`div16`) so
 *   nearer stripes appear taller — **all on the CPU**; the Amiga only paints flat
 *   colour per line.
 * - `SortStripes` is insertion sort on `z` (painter’s algorithm: back to front).
 * - Two full copper lists double-buffer **patches** only (`CopInsSet16`), not the
 *   whole list — cheap animation.
 *
 * Colour model:
 * - `colorShades` packs four base hues (`colorSet`) × 32 ramp entries: fade up from
 *   black, then fade to white. `SetLineColor` indexes with `color | l` where `l`
 *   is a lighting band from `z`.
 *
 * HRM (Copper WAIT, COLOR registers): https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * HRM mirror: http://amigadev.elowar.com/read/
 */
#include "effect.h"
#include "copper.h"
#include "fx.h"
#include "color.h"
#include <stdlib.h>

#define WIDTH   320
#define HEIGHT  256
/* Range for random initial (y,z) in GenerateStripes — keeps stripes near origin. */
#define SIZE    128
#define STRIPES 20
/* Border / clear colour (also end-of-list restore). */
#define BGCOL   0x204

/*
 * StripeT — one logical bar before/after projection.
 * - y, z: pre-rotation coordinates in fixed-point-ish space (see GenerateStripes).
 * - color: index 0..3 selecting which quarter of `colorShades` to use (×32 entries).
 */
typedef struct {
  short y, z;
  short color;
} StripeT;

static CopListT *cp[2];
/* lineColor[buffer][line] — CopIns handle for that line’s COLOR0 MOVE (patched each frame). */
static CopInsT *lineColor[2][HEIGHT];
static StripeT stripe[STRIPES];
static short active = 0;

static u_short colorSet[4] = { 0xC0F, 0xF0C, 0x80F, 0xF08 };
/* 4 hues × 32 shades: dark→sat→white ramps for lighting. */
static u_short colorShades[4 * 32];

/* Random initial bar seeds: clustered around center, random palette slot in 0x00/0x20/0x40. */
static void GenerateStripes(void) {
  short *s = (short *)stripe;
  short n = STRIPES;

  while (--n >= 0) {
    *s++ = (random() & (SIZE - 1)) - SIZE / 2;
    *s++ = (random() & (SIZE - 1)) - SIZE / 2;
    *s++ = random() & 0x60;
  }
}

/* Build 32-step ramps per hue: first half black→hue, second half hue→white. */
static void GenerateColorShades(void) {
  short i, j;
  u_short *s = colorSet;
  u_short *d = colorShades;

  for (i = 0; i < 4; i++) {
    u_short c = *s++;

    for (j = 0; j < 16; j++)
      *d++ = ColorTransition(0x000, c, j);
    for (j = 0; j < 16; j++)
      *d++ = ColorTransition(c, 0xfff, j);
  }
}

/*
 * One copper list: WAIT per line, then MOVE to COLOR0. `line[]` receives pointers
 * so Render can overwrite only the data words (fast). Trailing WAIT+colour restores
 * border after the playfield block.
 */
static CopListT *MakeCopperList(CopInsT **line) {
  CopListT *cp = NewCopList(HEIGHT * 2 + 100);
  short i;

  CopSetColor(cp, 0, BGCOL);

  for (i = 0; i < HEIGHT; i++) {
    CopWaitSafe(cp, Y(i), HP(16));
    *line++ = CopSetColor(cp, 0, 0);
  }

  CopWait(cp, Y(256), HP(16));
  CopSetColor(cp, 0, BGCOL);
  return CopListFinish(cp);
}

static void Init(void) {
  GenerateStripes();
  GenerateColorShades();

  cp[0] = MakeCopperList(lineColor[0]);
  cp[1] = MakeCopperList(lineColor[1]);

  CopListActivate(cp[0]);
}

static void Kill(void) {
  DeleteCopList(cp[0]);
  DeleteCopList(cp[1]);
}

/* “Camera” pivot for rotation + perspective (see div16 denominator). */
static short centerY = 0;
static short centerZ = 192;

/*
 * Project all stripes: read triples (y,z,color) from `s`, write (screenY, z, color)
 * into `d`. Uses fixed-point trig (SIN/COS >> 4) and `div16` for perspective.
 */
static void RotateStripes(short *d, short *s, short rotate) {
  short n = STRIPES;
  int cy = centerY << 8;
  short cz = centerZ;

  while (--n >= 0) {
    short sinA = SIN(rotate);
    short cosA = COS(rotate);
    short y = *s++;
    short z = *s++;
    int yp = (y * cosA - z * sinA) >> 4; 
    short zp = normfx(y * sinA + z * cosA);

    *d++ = div16(yp + cy, zp + cz);
    *d++ = zp;
    *d++ = *s++;
  }
}

/* Reset every line to background before drawing this frame’s bars (avoids trails). */
static void ClearLineColor(void) {
  CopInsT **line = lineColor[active];
  short n = HEIGHT;

  while (--n >= 0)
    CopInsSet16(*line++, BGCOL);
}

/*
 * Paint each stripe as a vertical column of constant COLOR0: top cap (darker shade),
 * `h` scanlines of core colour, bottom cap. `i` is top scanline; `h` from `z`.
 */
static void SetLineColor(short *s) {
  CopInsT **lines = lineColor[active];
  short n = STRIPES;
  u_short *shades = colorShades;

  while (--n >= 0) {
    short y = *s++;
    short z = *s++;
    u_short color = *s++;

    short h = (short)(z + 128) >> 5;
    short l = (z >> 2) + 16;
    short i = y + ((short)(HEIGHT - h) >> 1);

    if (l < 0)
      l = 0;
    if (l > 31)
      l = 31;

    if ((i >= 0) && (i + h < HEIGHT)) {
      CopInsT **line = &lines[i];
      short c0 = shades[color | l];
      short c1 = shades[color | (l >> 1)];

      h -= 2;

      CopInsSet16(*line++, c1);

      while (--h >= 0)
        CopInsSet16(*line++, c0);

      CopInsSet16(*line++, c1);
    }
  }
}

/*
 * Insertion sort by increasing z (draw far stripes first, near on top — though here
 * all spans are opaque colour lines, order mainly affects overlap appearance).
 * `register short n asm("d7")` pins the loop counter for the hand-tuned inner loop.
 */
static void SortStripes(StripeT *table) {
  StripeT *ptr = table + 1;
  register short n asm("d7") = STRIPES - 2;

  do {
    StripeT *curr = ptr;
    StripeT *prev = ptr - 1;
    StripeT this = *ptr++;
    while (prev >= table && prev->z > this.z)
      *curr-- = *prev--;
    *curr = this;
  } while (--n != -1);
}

static void RenderStripes(short rotate) {
  static StripeT temp[STRIPES];

  RotateStripes((short *)temp, (short *)stripe, rotate);
  SortStripes(temp);
  ClearLineColor();
  SetLineColor((short *)temp);
}

PROFILE(RenderStripes);

static void Render(void) {
  ProfilerStart(RenderStripes);
  {
    /* Slow rotation of the 3D field (phase drives SIN/COS in RotateStripes). */
    RenderStripes(SIN(frameCount * 4) * 2);
  }
  ProfilerStop(RenderStripes);

  CopListRun(cp[active]);
  TaskWaitVBlank();
  active ^= 1;
}

EFFECT(Stripes, NULL, NULL, Init, Kill, Render, NULL);
