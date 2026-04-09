/*
 * effects/main.c — single-effect harness (each effect .exe links one EffectT).
 *
 * Registers `VBlankInterrupt` to call `Effect.VBlank` if present; optional `BGTASK`
 * stress loop for multitask debugging. `main` runs the usual EffectLoad → Init → Run →
 * Kill lifecycle (see `system/effect.c`). UAE breakpoint hook may be present for WinUAE.
 *
 * HRM: https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 */
#include <custom.h>
#include <effect.h>
#include <uae.h>
#include <system/interrupt.h>
#include <system/task.h>

extern EffectT Effect;

/* Optional stress-test background task for scheduler/debug experiments. */
#define BGTASK 0

#if BGTASK
/* BgLoop — intentionally busy loop toggling COLOR0 to visualize background task time. */
static void BgLoop(__unused void *ptr) {
  Log("Inside background task!\n");
  for (;;) {
    custom->color[0] = 0xff0;
  }
}

static void StartBgTask(void) {
  static __aligned(8) char stack[256];
  static TaskT BgTask;

  TaskInit(&BgTask, "background", stack, sizeof(stack));
  TaskRun(&BgTask, 1, BgLoop, NULL);
}
#endif

/* VBlankISR — forwards vertical blank interrupts to effect-specific VBlank hook. */
static int VBlankISR(void) {
  if (Effect.VBlank)
      Effect.VBlank();
  return 0;
}

INTSERVER(VBlankInterrupt, 0, (IntFuncT)VBlankISR, NULL);

/* main — generic single-effect launcher used by many effect executables.
 * Lifecycle is framework-standard: Load -> Init -> Run -> Kill -> UnLoad. */
int main(void) {
  /* NOP that triggers fs-uae debugger to stop and inform GDB that it should
   * fetch segments locations to relocate symbol information read from file. */
  asm volatile("exg %d7,%d7");

  AddIntServer(INTB_VERTB, VBlankInterrupt);

#if BGTASK
  StartBgTask();
#endif

  EffectLoad(&Effect);
  EffectInit(&Effect);
  UaeWarpMode(0);
  EffectRun(&Effect);
  EffectKill(&Effect);
  EffectUnLoad(&Effect);

  RemIntServer(INTB_VERTB, VBlankInterrupt);

  return 0; 
}
