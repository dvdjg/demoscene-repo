/*
 * Vertical-blank (VBlank) wakeup integration for the mini-kernel.
 *
 * Purpose: on Amiga, INTB_VERTB fires once per displayed frame (after the
 * vertical blank interval). Effects often sync to that; here we register a
 * lightweight interrupt server that wakes a blocked task when a frame tick
 * occurs, so TaskWaitVBlank() can sleep without busy-waiting the CPU.
 *
 * Why use the interrupt instead of polling VPOSR: the CPU can do other work or
 * sleep cooperatively; polling the raster position burns cycles and is harder
 * to combine with multitasking.
 *
 * HRM (interrupts, copper timing): https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 */
#include <debug.h>
#include <system/interrupt.h>
#include <system/task.h>
#include <linkerset.h>

/*
 * IsWaiting — handshake between ISR and TaskWaitVBlank.
 * - 0: no one is sleeping on VBlank.
 * - Non-zero: TaskWaitVBlank() set us; handler clears to 0 and notifies the task.
 * Why a flag: the handler must be tiny; we only wake one waiter per frame here.
 */
static u_char IsWaiting = 0;

/*
 * VBlankWakeupHandler — runs in interrupt context (VERTB chain).
 * Calls TaskNotifyISR so the blocked task in TaskWait can resume.
 * C equivalent idea: set a volatile flag and poll in main — worse (latency, CPU).
 */
static void VBlankWakeupHandler(void) {
  if (IsWaiting) {
    IsWaiting = 0;
    TaskNotifyISR(INTF_VERTB);
  }
}

/* IntServer: priority 0, code = handler, data = NULL. See INTSERVER in interrupt.h */
INTSERVER(VertBlankWakeup, 0, (IntFuncT)VBlankWakeupHandler, NULL);

/*
 * TaskWaitVBlank — block current task until next vertical blank (MULTITASK only).
 * IntrDisable/Enable: avoid race where we set IsWaiting after the IRQ fired for this frame.
 */
void TaskWaitVBlank(void) {
  IntrDisable();
  IsWaiting = -1;
  TaskWait(INTF_VERTB);
  IntrEnable();
}

/*
 * InitVBlank — register server on VERTB chain at startup (ADD2INIT).
 */
void InitVBlank(void) {
  Log("[VBlank] Registered a wakeup handler.\n");
  AddIntServer(INTB_VERTB, VertBlankWakeup);
}

/*
 * KillVBlank — remove server on shutdown (ADD2EXIT).
 */
void KillVBlank(void) {
  RemIntServer(INTB_VERTB, VertBlankWakeup);
}

ADD2INIT(InitVBlank, 0);
ADD2EXIT(KillVBlank, 0);
