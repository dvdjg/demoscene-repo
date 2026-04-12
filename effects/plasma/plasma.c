/*
 * Plasma — Copper chunky: one COLOR change per 8×4 tile via COP2 / chunky buffer.
 *
 * CPU fills sin/cos tables and chunky buffer; copper paints pseudo-plasma at beam speed.
 * DIWHP/DIWVP overrides align copper WAIT quirks (see MakeCopperList). Uses copjmp2 path
 * described in body comments.
 *
 * HRM: https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * HRM mirror: http://amigadev.elowar.com/read/
 */
#include <effect.h>
#include <blitter.h>
#include <copper.h>
#include <fx.h>
#include <pixmap.h>
#include <system/memory.h>

#define WIDTH (320 - 32)
#define HEIGHT 256
#define DEPTH 0

#define HTILES (WIDTH / 8)
#define VTILES (HEIGHT / 4)

/*
 * Override beampos DIW so CopWait/CopSkip line math matches the copjmp2 / 8×4 tile
 * layout (see MakeCopperList comment block — copper chunky constraints).
 */
#undef DIWVP
#define DIWVP 0x2c
#undef DIWHP
#define DIWHP 0x88

/* chunky[bank][row] points to COLOR0 MOVE instruction for each 8x4 tile row. */
static CopInsT *chunky[2][VTILES];
/* Double-buffered copper lists (one displayed, one being updated). */
static CopListT *cp[2];
/* Active copper bank toggled each frame. */
static int active = 0;

#include "data/plasma-colors.c"

/* Precomputed sinusoidal tables used as cheap plasma basis functions. */
static char tab1[256], tab2[256], tab3[256];
/* Time/phase accumulators for X/Y components. */
static u_char a0, a1, a2, a3, a4;
/* Per-frame intermediate sums for each tile column/row. */
static u_char xbuf[HTILES], ybuf[VTILES];

/* GeneratePlasmaTables — one-time LUT generation in fixed-point domain. */
static void GeneratePlasmaTables(void) {
  short i;

  /* Generate plasma tables */
  for (i = 0; i < 256; i++) {
    short rad = i * 16;

    tab1[i] = fx4i(3 * 47) * SIN(rad * 2) >> 16;
    tab2[i] = fx4i(3 * 31) * COS(rad * 2) >> 16;
    tab3[i] = fx4i(3 * 37) * SIN(rad * 2) >> 16;
  }
}

static void Load(void) {
  /* Effect precompute stage; no CHIP allocations here. */
  GeneratePlasmaTables();
}

/* Double-buffered copper chunky with 8x4 pixels.
 *
 * In order to make copper run same stream of instructions (i.e. color line)
 * 4 times, the screen has to be constructed in a very specific way.
 *
 * Firstly, vertical start position must be divisible by 4. This is driven by
 * copper lack of capability to mask out the most significant bit in WAIT and
 * SKIP instruction. This causes a glitch after transition from line 127 to
 * line 128. The problem is mentioned in HRM:
 * http://amigadev.elowar.com/read/ADCD_2.1/Hardware_Manual_guide/node005B.html
 *
 * Secondly, a problem arises when transitioning from line 255 to line 256.
 * Inserted SKIP instruction effectively awaits beam position that won't ever
 * happen! To avoid it each SKIP instruction must be guaranteed to eventually
 * trigger. Thus they must be placed just before the end of scan line.
 * UAE copper debugger facility was used to find the right spot.
 */
static CopListT *MakeCopperList(CopInsT **row) {
  CopListT *cp = NewCopList(80 + (HTILES + 5) * VTILES);
  short x, y;

  CopWait(cp, Y(0), HP(0));

  for (y = 0; y < VTILES; y++) {
    CopInsPairT *location = CopMove32(cp, cop2lc, 0);
    CopInsT *label = CopWaitH(cp, Y(y * 4), X(-4));
    CopInsSet32(location, label);
    row[y] = CopSetColor(cp, 0, 0);
    for (x = 0; x < HTILES - 1; x++)
      CopSetColor(cp, 0, 0); /* Last CopIns finishes at HP=0xD2 */
    CopSetColor(cp, 0, 0); /* set background to black, finishes at HP=0xD6 */
    CopSkip(cp, Y(y * 4 + 3), LASTHP); /* finishes at HP=0xDE */
    CopMove16(cp, copjmp2, 0);
  }

  return CopListFinish(cp);
}

static void Init(void) {
  /* Build both copper buffers before first frame to avoid runtime allocation hitches. */
  cp[0] = MakeCopperList(chunky[0]);
  cp[1] = MakeCopperList(chunky[1]);

  CopListActivate(cp[1]);
}

static void Kill(void) {
  DisableDMA(DMAF_COPPER);

  DeleteCopList(cp[0]);
  DeleteCopList(cp[1]);
}

/* UpdateXBUF — evaluate X-dependent plasma term for each 8-pixel tile column. */
static void UpdateXBUF(void) {
  u_char _a0 = a0;
  u_char _a1 = a1;
  u_char _a2 = a2;
  u_char *_buf = xbuf;
  short n = HTILES - 1;

  do {
    *_buf++ = tab1[_a0] + tab2[_a1] + tab3[_a2];
    _a0 += 1; _a1 += 2; _a2 += 3;
  } while (--n >= 0);
}

/* UpdateYBUF — evaluate Y-dependent plasma term for each 4-line tile row. */
static void UpdateYBUF(void) {
  u_char _a3 = a3;
  u_char _a4 = a4;
  u_char *_buf = ybuf;
  short n = VTILES - 1;

  do {
    *_buf++ = tab1[_a3] + tab2[_a4];
    _a3 += 2; _a4 += 3;
  } while (--n >= 0);
}

#define OPTIMIZED 1

static void UpdateChunky(void) {
  u_short *cmap = colors.pixels;
  short y;

  UpdateXBUF();
  UpdateYBUF();

  a0 += 1; a1 += 3; a2 += 2; a3 += 1; a4 -= 1;

  /* Fill COLOR0 words in the inactive copper list from xbuf+ybuf sums. */
  for (y = 0; y < VTILES; y++) {
    short *ins = &chunky[active][y]->move.data;

    short x = HTILES - 1;
    do {
#if OPTIMIZED
      /* Inline asm variant avoids extra loads/stores in inner loop. */
      /* %%d0: en inline asm, %% -> % para que gas reciba %d0 (coherente con %d1/%a2…). */
      asm volatile("moveq #0,%%d0\n"
                   "moveb %1@(%2:w),%%d0\n"
                   "addb  %3@(%4:w),%%d0\n"
                   "addw  %%d0,%%d0\n"
                   "movew %5@(%%d0:w),%0@+\n"
                   "addql #2,%0\n"
                   : "+a" (ins)
                   : "a" (xbuf), "d" (x), "a" (ybuf), "d" (y), "a" (cmap)
                   : "d0");
#else
      /* Straight C equivalent of inner loop (slower on 68000). */
      u_char v = xbuf[x] + ybuf[y];
      *ins++ = cmap[v];
      ins++;
#endif
    } while (--x >= 0);
  }
}

PROFILE(RenderPlasma);

static void Render(void) {
  ProfilerStart(RenderPlasma);
  {
    UpdateChunky();
  }
  ProfilerStop(RenderPlasma);

  /* Run newly prepared copper list during this frame. */
  CopListRun(cp[active]);
  TaskWaitVBlank();
  active ^= 1;
}

EFFECT(Plasma, Load, NULL, Init, Kill, Render, NULL);
