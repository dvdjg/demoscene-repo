/*
 * effect.h — Sistema de efectos para demos/juegos en Amiga.
 *
 * Cada efecto se registra con EFFECT() y define: Load (precalcular en background),
 * UnLoad, Init (reservar memoria, copper, DMA), Kill, Render (un frame), VBlank (opcional).
 * El bucle principal llama EffectLoad → EffectInit → EffectRun (Render cada frame) → EffectKill → EffectUnLoad.
 */
/*
 * English tutorial supplement:
 * An Effect is a self-contained scene: load assets, grab CHIP RAM, configure the
 * copper/blitter/audio, then render each frame until exit. Splitting demos this way
 * keeps memory and hardware setup modular. See system/effect.c for lifecycle glue.
 */
#ifndef __EFFECT_H__
#define __EFFECT_H__

#include <config.h>
#include <types.h>
#include <string.h>
#include <stab.h>
#include <debug.h>

/*
 * frameCount — global frame counter (typically 50 Hz PAL), advanced by the
 * effect loop / CIA frame counter. Used for animation phase and profiling.
 */
extern short frameCount;

/*
 * ReadFrameCount — same value as frameCount but safe to call from VBlank ISR
 * context where the global might be updated concurrently (see system/effect.c).
 */
short ReadFrameCount(void);

/*
 * lastFrameCount — previous frame index seen by EffectRun; difference from
 * frameCount measures frame time. Used by profiler and frame-skipping logic.
 */
extern short lastFrameCount;

/*
 * exitLoop — when true, EffectRun stops calling Render (e.g. left mouse).
 * Writable by input or effects for scripted exits.
 */
extern bool exitLoop;

#ifdef INTRO
/* Intro build: timeline from intro start / time until end (sync). */
extern short frameFromStart;
extern short frameTillEnd;
#endif

#ifndef DEMO
void TimeWarp(u_short frame);
#else
/* DEMO build: no time warp (macro expands to nothing). */
#define TimeWarp(x)
#endif

typedef void (*EffectFuncT)(void);

/*
 * EFFECT_MAGIC — ASCII 'GTN!' in a u_int; loader/debugger checks that an
 * EffectT really is an effect (not random memory).
 */
#define EFFECT_MAGIC 0x47544e21 /* GTN! */

typedef struct Effect {
  /* Filled with 'GTN!' - marker for loader checks. */
  u_int magic;
  /* Effect C-symbol dumped to string. */
  const char *name;
  /*
   * Executed in background task when other effect is running.
   * Precalculates data for the effect to be launched.
   */
  union {
    EffectFuncT Func;
    intptr_t Status;
  } Load;
  /*
   * Frees all resources allocated by "Load" step.
   */
  EffectFuncT UnLoad;
  /*
   * Does all initialization steps required to launch the effect:
   * 1) Allocate memory for buffers
   * 2) Generate copper lists
   *    (setup for display window, display data fetch, palette, sprites, etc.)
   * 3) Set up interrupts and DMA channels (copper, blitter, etc.)
   */
  union {
    EffectFuncT Func;
    intptr_t Status;
  } Init;
  /*
   * Frees all resources allocated by "Init" step.
   */
  EffectFuncT Kill;
  /*
   * Renders single frame of an effect.
   */
  EffectFuncT Render;
  /*
   * Called each frame during VBlank interrupt.
   */
  EffectFuncT VBlank;
} EffectT;

bool EffectIsRunning(EffectT *effect);
void EffectLoad(EffectT *effect);
void EffectInit(EffectT *effect);
void EffectKill(EffectT *effect);
void EffectUnLoad(EffectT *effect);
void EffectRun(EffectT *effect);

/*
 * EFFECT(NAME, L, U, I, K, R, V) — defines a static EffectT named NAME##Effect.
 * Parameters: L=Load, U=UnLoad, I=Init, K=Kill, R=Render, V=VBlank (NULL ok).
 * Expands to: `__code EffectT LoaderEffect = { .magic = ..., .name = "Loader", ... };`
 * __code places the struct in the code section (read-only after link) if supported.
 */
#define EFFECT(NAME, L, U, I, K, R, V)                                         \
  __code EffectT NAME##Effect = {                                              \
    .magic = EFFECT_MAGIC,                                                     \
    .name = #NAME,                                                             \
    .Load = { .Func = (L) },                                                   \
    .UnLoad = (U),                                                             \
    .Init = { .Func = (I) },                                                   \
    .Kill = (K),                                                               \
    .Render = (R),                                                             \
    .VBlank = (V)                                                              \
  };

typedef struct Profile {
  const char *name;
  u_int lines, total;
  u_short min, max;
  u_short count;
  u_short reportFrame;
} ProfileT;

#ifdef PROFILER
/*
 * PROFILE(NAME) — static ProfileT instance for block NAME; paired with
 * ProfilerStart/Stop around hot code. Uses raster line counter (see profiler.c).
 */
#define PROFILE(NAME)                                                          \
  static ProfileT *_##NAME##_profile = &(ProfileT){                            \
    .name = #NAME,                                                             \
    .lines = 0,                                                                \
    .total = 0,                                                                \
    .min = 65535,                                                              \
    .max = 0,                                                                  \
    .count = 0,                                                                \
    .reportFrame = 0,                                                          \
  };

#define ProfilerStart(NAME) _ProfilerStart(_##NAME##_profile)
#define ProfilerStop(NAME) _ProfilerStop(_##NAME##_profile)
#else
#define PROFILE(NAME)
#define ProfilerStart(NAME)
#define ProfilerStop(NAME)
#endif

#ifdef MULTITASK
/* Puts a task into sleep waiting for Vertical Blank interrupt.
 * Let's background task do its job. */
void TaskWaitVBlank(void);
#else
/* Single-task: busy-wait vertical position (see system/amigaos WaitVBlank or custom). */
#define TaskWaitVBlank WaitVBlank
#endif

void _ProfilerStart(ProfileT *prof);
void _ProfilerStop(ProfileT *prof);

#endif /* !__EFFECT_H__ */
