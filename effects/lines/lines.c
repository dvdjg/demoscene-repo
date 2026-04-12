/*
 * Effect: Lines — benchmark / demo of line drawing: Blitter line mode vs CPU Bresenham.
 *
 * Compile-time LINE selects implementation (see table below). The effect draws a
 * fan of lines from corners; profiling macros measure cost per frame.
 * HRM: Blitter line mode — https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * HRM mirror: http://amigadev.elowar.com/read/
 */
#include "effect.h"
#include "blitter.h"
#include "copper.h"
#include "line.h"

/* Display size (lores). */
#define WIDTH 320
#define HEIGHT 256
/* Single bitplane: monochrome line test pattern. */
#define DEPTH 1

/*
 * 0 -> BlitterLine (1706)
 * 1 -> CpuLine (11245)
 * 2 -> CpuEdge (8550)
 * 3 -> CpuLineOpt (10872)
 * 4 -> CpuEdgeOpt (8253)
 */
#define LINE 0

/* External optimized routines (usually hand-tuned asm).
 * They are ABI-compatible with C variants, but keep arguments in fixed
 * registers to reduce overhead in the inner benchmark loop. */
void CpuEdgeOpt(void *bpl __ASM_REG_PARM("a0"), short stride __ASM_REG_PARM("a1"),
                short xs __ASM_REG_PARM("d0"), short ys __ASM_REG_PARM("d1"),
                short xe __ASM_REG_PARM("d2"), short ye __ASM_REG_PARM("d3"));


void CpuLineOpt(void *bpl __ASM_REG_PARM("a0"), short stride __ASM_REG_PARM("a1"),
                short xs __ASM_REG_PARM("d0"), short ys __ASM_REG_PARM("d1"),
                short xe __ASM_REG_PARM("d2"), short ye __ASM_REG_PARM("d3"));

/* Single bitplane framebuffer for line drawing. */
static BitmapT *screen;
/* Copper list: bitplane pointers only (no per-line colour tricks). */
static CopListT *cp;

/*
 * Load — allocate screen, set default colours, build minimal copper for bitplanes.
 */
static void Load(void) {
  /* One bitplane is enough for a geometry benchmark and minimizes DMA traffic. */
  screen = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR);

  SetupPlayfield(MODE_LORES, DEPTH, X(0), Y(0), WIDTH, HEIGHT);
  SetColor(0, 0x000);
  SetColor(1, 0xfff);

  cp = NewCopList(100);
  CopSetupBitplanes(cp, screen, DEPTH);
  CopListFinish(cp);
}

/*
 * UnLoad — free copper list and bitmap (pair of Load).
 */
static void UnLoad(void) {
  DeleteCopList(cp);
  DeleteBitmap(screen);
}

/*
 * Init — activate copper and enable blitter + raster DMA for chosen line backend.
 */
static void Init(void) {
  /* BLITHOG biases arbitration toward blitter so LINE==0 can be profiled under
   * favorable hardware conditions, as done in many old-school performance tests. */
  CopListActivate(cp);
  EnableDMA(DMAF_BLITTER | DMAF_RASTER | DMAF_BLITHOG);
}

PROFILE(Lines);

/*
 * Render — call Setup once per frame, then draw two sets of fan lines (corner to edge).
 *
 * First loop: vertical sweep (x varies). Second loop: horizontal sweep (y varies).
 * Spacing i+=2 reduces line count. Choice of line drawer is #if LINE.
 */
static void Render(void) {
  ProfilerStart(Lines);
  {
    short i;

#if LINE == 2
    CpuEdgeSetup(screen, 0);
#elif LINE == 1
    CpuLineSetup(screen, 0);
#elif LINE == 0
    BlitterLineSetup(screen, 0, LINE_OR|LINE_SOLID);
#endif

    for (i = 0; i < screen->width; i += 2) {
#if LINE == 4
      CpuLineOpt(screen->planes[0], screen->bytesPerRow,
                 i, 0, screen->width - 1 - i, screen->height - 1);
#elif LINE == 3
      CpuEdgeOpt(screen->planes[0], screen->bytesPerRow,
                 i, 0, screen->width - 1 - i, screen->height - 1);
#elif LINE == 2
      CpuEdge(i, 0, screen->width - 1 - i, screen->height - 1);
#elif LINE == 1
      CpuLine(i, 0, screen->width - 1 - i, screen->height - 1);
#elif LINE == 0
      BlitterLine(i, 0, screen->width - 1 - i, screen->height - 1);
#endif
    }

    for (i = 0; i < screen->height; i += 2) {
#if LINE == 4
      CpuLineOpt(screen->planes[0], screen->bytesPerRow,
                 0, i, screen->width - 1, screen->height - 1 - i);
#elif LINE == 3
      CpuEdgeOpt(screen->planes[0], screen->bytesPerRow,
                 0, i, screen->width - 1, screen->height - 1 - i);
#elif LINE == 2
      CpuEdge(0, i, screen->width - 1, screen->height - 1 - i);
#elif LINE == 1
      CpuLine(0, i, screen->width - 1, screen->height - 1 - i);
#elif LINE == 0
      BlitterLine(0, i, screen->width - 1, screen->height - 1 - i);
#endif
    }
  }
  ProfilerStop(Lines);
}

/* Load/UnLoad/Init/Render wired into the effect framework; no VBlank hook. */
EFFECT(Lines, Load, UnLoad, Init, NULL, Render, NULL);
