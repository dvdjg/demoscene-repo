/*
 * Loader effect — progress bar while modules/samples load, with ProTracker audio.
 *
 * Purpose: first-screen experience: draws a frame with CpuLine (CPU-drawn lines),
 * blits the static loader bitmap once, then uses the copper to display bitplanes.
 * PtInit drives the music (LoaderModule / LoaderSamples from embedded data).
 *
 * Why CpuLine + blitter: the border/outline uses fast line drawing; the logo/art
 * comes from a preconverted bitmap (blitter copy is the standard OCS way to
 * stamp CHIP RAM). Raster DMA enables the copper-driven display.
 *
 * HRM (bitplanes, copper, blitter): https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 */
#include <effect.h>
#include <copper.h>
#include <blitter.h>
#include <gfx.h>
#include <line.h>
#include <ptplayer.h>
#include <color.h>

#define _SYSTEM
#include <system/cia.h>

#include "data/loader.c"

/* Embedded ProTracker replay assets (linked from packed resources). */
#if defined(__ELF__)
extern u_char _LoaderModule[];
extern u_char _LoaderSamples[];
#else
extern u_char LoaderModule[];
extern u_char LoaderSamples[];
#endif

/* Runtime framebuffer (CHIP) shown during loading. */
static __code BitmapT *screen;
/* Copper list that points display hardware to `screen` bitplanes. */
static __code CopListT *cp;

/* Progress bar rectangle in screen-space pixels. */
#define X1 99
#define Y1 230
#define X2 220
#define Y2 238

/* Loader display mode: 320x256, 3 bitplanes (8 colours). */
#define WIDTH 320
#define HEIGHT 256
#define DEPTH 3

/*
 * Init — boot audio replay and prepare loader screen.
 *
 * Steps:
 * 1) Install CIA timer interrupt for ProTracker and start module playback.
 * 2) Allocate bitmap/copper, draw frame border with CpuLine.
 * 3) Configure playfield + palette, blit static loader artwork.
 * 4) Activate copper list and enable raster DMA.
 *
 * Why this split: expensive static artwork is copied once; per-frame Render only
 * updates the progress bar, which keeps loader timing stable during I/O.
 */
static void Init(void) {
  PtInstallCIA();
#if defined(__ELF__)
  PtInit(_LoaderModule, _LoaderSamples, 1);
#else
  PtInit(LoaderModule, LoaderSamples, 1);
#endif
  PtEnable = 1;

  screen = NewBitmap(WIDTH, HEIGHT, DEPTH, BM_CLEAR);
  cp = NewCopList(40);

  CpuLineSetup(screen, 0);
  CpuLine(X1 - 1, Y1 - 2, X2 + 1, Y1 - 2);
  CpuLine(X1 - 1, Y2 + 1, X2 + 1, Y2 + 1);
  CpuLine(X1 - 2, Y1 - 1, X1 - 2, Y2 + 1);
  CpuLine(X2 + 2, Y1 - 1, X2 + 2, Y2 + 1);

  SetupPlayfield(MODE_LORES, DEPTH, X(0), Y(0), WIDTH, HEIGHT);
  LoadColors(loader_colors, 0);

  EnableDMA(DMAF_BLITTER);
  BitmapCopy(screen, 0, 0, &loader);
  WaitBlitter();
  DisableDMA(DMAF_BLITTER);

  CopSetupBitplanes(cp, screen, DEPTH);
  CopListFinish(cp);
  CopListActivate(cp);

  EnableDMA(DMAF_RASTER);
}

/*
 * Kill — stop display/audio subsystems and free loader resources.
 *
 * BlitterStop/CopperStop first prevent hardware DMA from reading freed memory.
 * CIA replay hook is removed after PtEnd to leave system timers in clean state.
 */
static void Kill(void) {
  BlitterStop();
  CopperStop();

  DeleteCopList(cp);
  DeleteBitmap(screen);

  PtEnd();
  PtRemoveCIA();
  DisableDMA(DMAF_AUDIO);
}

/*
 * Render — grow progress bar according to frameCount.
 *
 * `x` persists across frames (function-static) so only new columns are drawn.
 * This avoids redrawing the full bar each frame and keeps CPU cost minimal while
 * assets continue loading in the background.
 */
static void Render(void) {
  /* Current filled width in pixels relative to X1. */
  static __code short x = 0;
  /* Slow down progression: 1 pixel every 8 frames. */
  short newX = frameCount >> 3;
  if (newX > 121)
    newX = 122;
  /* Draw only the newly revealed vertical slices. */
  for (; x < newX; x++) {
    CpuLine(X1 + x, Y1, X1 + x, Y2);
  }
}

/* Loader effect lifecycle: only Init/Kill/Render are needed here. */
EFFECT(Loader, NULL, NULL, Init, Kill, Render, NULL);
