/*
 * Floor — Mode-7-ish stripe floor: copper palette + per-line BPLCON1 fine scroll.
 *
 * The playfield uses a pre-rendered grayscale `floor` bitmap (data/floor.c). Colour
 * “neon” comes entirely from the palette: each horizontal band (every 8 scanlines)
 * patches colour 1–15 in the copper list. `stripeLight` selects a depth cue per line
 * into `colortab`; `ShiftColors` rotates which logical stripe feeds which pen so bands
 * appear to slide. `ControlStripes` animates each stripe’s RGB toward on/off with
 * `ColorTransition` and timed `step` counters.
 *
 * Horizontal perspective / skew: `stripeWidth` (per line) indexes into
 * `shifterValues[offset][line]` — precomputed BPLCON1 nibbles so `ShiftStripes` only
 * pokes one byte per CopIns per frame (fast path into the MOVE’s data).
 *
 * Double copper lists: the CPU rewrites hundreds of CopIns per frame. While one list
 * is being fetched by COP DMA, the other can be patched without fighting the beam;
 * `CopListRun` points COP1LC at the list that is finished for the *next* refresh.
 * (Single-buffering would risk the Copper seeing half-updated MOVEs mid-frame.)
 *
 * HRM: https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * HRM mirror: http://amigadev.elowar.com/read/
 */

#include <effect.h>
#include <color.h>
#include <copper.h>
#include <fx.h>
#include <gfx.h>

#include <stdlib.h>
#include <system/memory.h>

#define WIDTH 320
#define HEIGHT 256
#define DEPTH 4

static CopListT *cp[2];
static CopInsT *copLine[2][HEIGHT];
static short active = 0;

/* A struct that controls stripe's colors */
typedef struct {
  short step; /* when transition to the next color in the cycle */
  short orig; /* initial color value from which we start the color cycle */
  short color; /* current stripe's color */
} StripeT;

static StripeT stripe[15];
/* A table for storing colors as they are being shifted */
static short rotated[15];
static u_char shifterValues[16][HEIGHT];

#include "data/stripes.c"
#include "data/floor.c"
/* Width of the leftmost stripe (in pixels) at any given scanline */
#include "data/stripeWidth.c"
/*
 * Light level (values [0-11] where 11 is the darkest) at any given scanline,
 * used to depth to the stripe's colors
 */
#include "data/stripeLight.c"

/*
 * This one is a bit tricky - generate a table of values that will be written
 * to BPLCON1 (PF1Px and PF2Px bits) while shifting the playfields. If you are
 * wondering why this table is so big and has a lot of repeating values - it
 * makes fetching shift values much faster and easier in the ShiftStripes()
 * function that is executed each frame.
 */
static void GenerateShifterValues(void) {
  u_char *data = (u_char *)shifterValues;
  short i, j;
  
  /*
   * BPLCON1 packs PF1Px in the low nibble and PF2Px in the high nibble. Using the
   * same value in both nibbles keeps PF1 and PF2 fine-scroll in lockstep — required
   * here so all four bitplanes stay coherent when we skew the whole playfield.
   * Precomputing 16 × HEIGHT rows avoids per-frame math in the inner loop; ShiftStripes
   * only indexes (offset, line) and reads one byte.
   */
  for (j = 0; j < 16; j++) {
    for (i = 0; i < HEIGHT; i++) {
      short s = 1 + ((i * j) >> 8);
      *data++ = (s << 4) | s;
    }
  }
}

/*
 * One WAIT + BPLCON1 per line (patched by ShiftStripes); every 8th line allocates
 * 15 CopSetColor slots — ColorizeStripes later sets MOVE data for pens 1..15.
 */
static CopListT *MakeCopperList(CopInsT **line) {
  CopListT *cp = NewCopList(100 + 2 * HEIGHT + 15 * HEIGHT / 8);
  short i;

  CopSetupBitplanes(cp, &floor, DEPTH);

  for (i = 0; i < HEIGHT; i++) {
    CopWait(cp, Y(i), HP(0));
    line[i] = CopMove16(cp, bplcon1, 0);

    /*
     * Colours are not patched on every scanline — only every 8th line we emit 15
     * MOVEs (pens 1–15). ColorizeStripes then walks in steps of 8 `line` pointers
     * and uses `*line + pen` so each pen’s CopIns sits contiguously after that
     * line’s BPLCON1 instruction. Fewer copper instructions than 15×HEIGHT MOVEs.
     */
    if ((i & 7) == 0) {
      short j;

      for (j = 1; j < 16; j++)
        CopSetColor(cp, j, 0);
    }
  }

  return CopListFinish(cp);
}

static void InitStripes(void) {
  StripeT *s = stripe;
  short n = 15;

  while (--n >= 0) {
    s->step = -16 * (random() & 7);
    s->orig = stripes_colors[random() & 3];
    /* Every stripe starts black */
    s->color = 0;
    s++;
  }
}

static void Init(void) {
  GenerateShifterValues();
  InitStripes();

  SetupMode(MODE_LORES, DEPTH);
  SetupDisplayWindow(MODE_LORES, X(0), Y(0), WIDTH, HEIGHT);
  /*
   * Fetch starts 16 pixels left of the DIW and fetches WIDTH+16 visible pixels.
   * That margin is headroom for BPLCON1 horizontal shifts: shifting consumes pixels
   * from the “invisible” margin instead of leaving holes at the right edge.
   */
  SetupBitplaneFetch(MODE_LORES, X(-16), WIDTH + 16);
  SetColor(0, 0);

  cp[0] = MakeCopperList(copLine[0]);
  cp[1] = MakeCopperList(copLine[1]);

  /* Start displaying the list we are *not* about to patch first (active==0). */
  CopListActivate(cp[active ^ 1]);
  EnableDMA(DMAF_RASTER);
}

static void Kill(void) {
  DisableDMA(DMAF_RASTER);
  DeleteCopList(cp[0]);
  DeleteCopList(cp[1]);
}

/*
 * Rotate which physical stripe feeds which colour register (1–15). Only the coarse
 * component offset/16 is used so palette bands slide slowly; fine motion is handled
 * separately by BPLCON1 (ShiftStripes). Splitting coarse/fine avoids fighting the
 * same parameter for two different visual effects.
 */
static void ShiftColors(short offset) {
  short *dst = rotated;
  short n = 15;
  short i = 0;

  offset = (offset / 16) % 15;

  while (--n >= 0) {
    short c = i++ - offset;
    if (c < 0)
      c += 15;
    *dst++ = stripe[c].color;
  }
}

/*
 * Apply rotated stripe RGB through stripeLight bands (8 lines per step).
 */
static void ColorizeStripes(CopInsT **stripeLine) {
  short i;

  for (i = 1; i < 16; i++) {
    CopInsT **line = stripeLine;
    short *light = stripeLight;
    short n = HEIGHT / 8;
    short r, g, b;

    {
      /* Get the offseted color of the stripe */
      short s = rotated[i - 1];

      /* Extract RGB values from color */
      r = s & 0xf00;
      s <<= 4;
      g = s & 0xf00;
      s <<= 4;
      b = s & 0xf00;
    }

    /*
     * stripeLight picks a row in colortab; same base RGB gets remapped per 8-line
     * band so stripes read as “closer / farther” without repainting the floor bitmap.
     */
    while (--n >= 0) {
      u_char *tab = colortab + (*light);
      short color = (tab[r] << 4) | (u_char)(tab[g] | (tab[b] >> 4));

      CopInsSet16(*line + i, color);

      /* Increment the pointers */
      line += 8; light += 8;
    }
  }
}

/*
 * Patch each line’s BPLCON1 immediate. CopIns MOVE stores `reg` then `data` as
 * big-endian shorts; scroll values here fit in the low byte of `move.data`, so
 * ptr[3] updates only that byte (high byte stays 0). Cheaper than CopInsSet16 in
 * the per-line loop — see #if 0 for the word-sized alternative.
 *
 * stripeWidth chooses which column of shifterValues to use on each scanline so the
 * skew varies with Y (pseudo-perspective); offset picks the animation frame row.
 */
static void ShiftStripes(CopInsT **line, short offset) {
  short *width = stripeWidth;
  u_char *data = (u_char *)shifterValues;
  u_char *ptr;
  short n = HEIGHT;

  offset = (offset & 15) << 8;
  data += offset;
  
  while (--n >= 0) {
#if 0
    CopInsSet16(*line++, data[*width++]);
#else
    ptr = (u_char *)(*line++);
    ptr[3] = data[*width++];
#endif
  }
}

static void ControlStripes(void) {
  StripeT *s = stripe;
  /* If the main loop misses frames, advance animation by real elapsed frames. */
  short diff = frameCount - lastFrameCount;
  short n = 15;

  while (--n >= 0) {
    s->step -= diff;
    if (s->step < -128) {
      /* 
       * If we've reached the end of the cycle,
       * start it again at a random point.
       */
      s->step = 64 + (random() & 255);
    }

    if (s->step >= 0) {
      short step = s->step / 8;
      short from, to;

      if (step > 15) {
        /* Make color go brighter */
        from = s->orig;
        to = 0xfff;
        step -= 16;
      } else {
        /* Start going back to the original color */
        from = 0x000;
        to = s->orig;
      }
      s->color = ColorTransition(from, to, step);
    }

    s++;
  }
}

PROFILE(Floor);

static void Render(void) {
  ProfilerStart(Floor);
  {
    /*
     * Order matters: ControlStripes updates stripe[].color used by ShiftColors into
     * rotated[]; ColorizeStripes consumes rotated[]; ShiftStripes only touches BPLCON1.
     */
    short offset = normfx(SIN(frameCount * 8) * 1024) + 1024;
    CopInsT **line = copLine[active];

    ControlStripes();
    ShiftColors(offset);
    ColorizeStripes(line);
    ShiftStripes(line, offset);
  }
  ProfilerStop(Floor);

  CopListRun(cp[active]);
  TaskWaitVBlank();
  active ^= 1;
}

EFFECT(Floor, NULL, NULL, Init, Kill, Render, NULL);
