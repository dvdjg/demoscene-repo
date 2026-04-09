/*
 * Effect lifecycle helpers: load/init/render/kill/unload and frame counting.
 *
 * Purpose: each demo "effect" is described by an EffectT (see include/effect.h)
 * with optional Load/Init/Render/Kill hooks. This module sequences those calls,
 * tracks a global frame counter (synced to the CIA line counter), and optional
 * debug hooks (memory stats, remote status).
 *
 * Why centralize: the main binary stays small; effects register through tables
 * and share one timing/render loop pattern.
 */
#include <custom.h>
#include <effect.h>
#include <system/cia.h>

/* frameCount — current animation frame index (from CIA frame counter in EffectRun). */
short frameCount = 0;
/* lastFrameCount — previous counter value; paired with frameCount to detect new frame. */
short lastFrameCount = 0;
/* exitLoop — set by LeftMouseButton() each iteration; user exits effect when true. */
bool exitLoop = false;

/* SHOW_MEMORY_STATS — compile-time switch: log MemAvail after each lifecycle step. */
#define SHOW_MEMORY_STATS 0
#define REMOTE_CONTROL 0

#if SHOW_MEMORY_STATS
# include <system/memory.h>
static void ShowMemStats(void) {
  Log("[Memory] CHIP: %d/%d FAST: %d/%d\n",
      MemAvail(MEMF_CHIP|MEMF_LARGEST), MemAvail(MEMF_CHIP),
      MemAvail(MEMF_FAST|MEMF_LARGEST), MemAvail(MEMF_FAST));
}
#else
# define ShowMemStats()
#endif

#if REMOTE_CONTROL
# include <system/file.h>
static void SendEffectStatus(EffectT *effect) {
  /* NOTE: possible issue: `effect->state` is not a field of EffectT — this branch
   * only compiles if REMOTE_CONTROL is enabled; verify before use. */
  FilePrintf("ES %s %d\n", effect->name, effect->state);
}
#else
# define SendEffectStatus(x)
#endif

/* DONE — bit stored in Load.Status / Init.Status unions to mean "step completed". */
#define DONE 1

/*
 * EffectIsRunning — true if Init has completed (Init.Status has DONE bit).
 * Uses Status as bitmask in the Func/Status union — same storage as function pointer.
 */
bool EffectIsRunning(EffectT *effect) {
  return (effect->Init.Status & DONE) ? true : false; 
}

/*
 * EffectLoad — run optional Load.Func once (background precalc), mark Load.Status DONE.
 */
void EffectLoad(EffectT *effect) {
  if (effect->Load.Status & DONE)
    return;

  if (effect->Load.Func) {
    Log("[Effect] Loading '%s'\n", effect->name);
    effect->Load.Func();
    ShowMemStats();
  }

  effect->Load.Status |= DONE; 
  SendEffectStatus(effect);
}

/*
 * EffectInit — allocate buffers, copper, etc. (Init.Func), mark Init.Status DONE.
 */
void EffectInit(EffectT *effect) {
  if (effect->Init.Status & DONE)
    return;

  if (effect->Init.Func) {
    Log("[Effect] Initializing '%s'\n", effect->name);
    effect->Init.Func();
    ShowMemStats();
  }

  effect->Init.Status |= DONE;
  SendEffectStatus(effect);
}

/*
 * EffectKill — reverse Init: free hardware (Kill), clear DONE from Init.Status with XOR.
 */
void EffectKill(EffectT *effect) {
  if (!(effect->Init.Status & DONE))
    return;

  if (effect->Kill) {
    Log("[Effect] Killing '%s'\n", effect->name);
    effect->Kill();
    ShowMemStats();
  }

  effect->Init.Status ^= DONE;
  SendEffectStatus(effect);
}

/*
 * EffectUnLoad — reverse Load (UnLoad), clear DONE from Load.Status.
 */
void EffectUnLoad(EffectT *effect) {
  if (!(effect->Load.Status & DONE))
    return;

  if (effect->UnLoad) {
    Log("[Effect] Unloading '%s'\n", effect->name);
    effect->UnLoad();
    ShowMemStats();
  }

  effect->Load.Status ^= DONE;
  SendEffectStatus(effect);
}

/*
 * ReadFrameCount — thin alias to CIA frame counter (see drivers/cia-frame.c).
 */
short ReadFrameCount(void) {
  return ReadFrameCounter();
}

/*
 * EffectRun — main loop: sync frameCount to CIA counter; call Render once per new frame
 * until exitLoop. Why compare lastFrameCount != frameCount: skips duplicate renders if
 * counter did not advance (e.g. same IRQ tick).
 */
void EffectRun(EffectT *effect) {
  SetFrameCounter(frameCount);

  lastFrameCount = ReadFrameCounter();

  do {
    int t = ReadFrameCounter();
    exitLoop = LeftMouseButton();
    frameCount = t;
    if ((lastFrameCount != frameCount) && effect->Render)
      effect->Render();
    lastFrameCount = t;
  } while (!exitLoop);
}

/*
 * TimeWarp — non-DEMO: jump logical frame (sync music). DEMO build: macro no-op in header.
 */
void TimeWarp(u_short frame) {
  frameCount = frame;
}
