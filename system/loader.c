/*
 * Bare-metal Loader: memory, CPU model, exceptions, interrupts, then main().
 *
 * Purpose: after the boot sector (or AmigaOS SaveOS) hands us BootDataT, we
 * register chip/fast memory regions, install exception and interrupt vectors,
 * reset CIA timers to a known idle state, enable master DMA/INT, and run
 * linker-generated init functions before main(). This is the hub between the
 * platform and the demoscene demo.
 *
 * Why own the vectors: demos need deterministic IRQ dispatch (VBlank, disk,
 * etc.) without AmigaOS remapping handlers underneath us.
 *
 * HRM (CIA, DMA, interrupts): https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 */
#include <config.h>
#include <debug.h>
#include <system/amigahunk.h>
#include <system/autoinit.h>
#include <system/boot.h>
#include <system/cia.h>
#include <system/cpu.h>
#include <system/exception.h>
#include <system/interrupt.h>
#include <system/memory.h>
#include <system/task.h>

/* CpuModel/BootDev become global runtime facts used by subsystems/drivers. */
u_char CpuModel = CPU_68000;
u_char BootDev;

extern int main(void);

static void IdleTaskLoop(__unused void *ptr) {
  for (;;) {
    Debug("Processor goes asleep with SR=%04x!", 0x2000);
    CpuWait();
  }
}

static __aligned(8) char IdleTaskStack[256];
static TaskT IdleTask;

/* Loader — platform bring-up entry point called from crt0/boot handoff.
 * Sequence:
 * 1) capture crash/boot context, register memory arenas,
 * 2) install exception + interrupt vectors,
 * 3) reset CIA and global interrupt/DMA state,
 * 4) run init list, call main, run exit list. */
void Loader(BootDataT *bd) {
  CrashInit(bd);

  Log("[Loader] VBR at $%08x\n", (u_int)bd->bd_vbr);
  Log("[Loader] CPU model $%02x\n", bd->bd_cpumodel);
  Log("[Loader] Stack at $%08x (%d bytes)\n",
      (u_int)bd->bd_stkbot, bd->bd_stksz);
  Log("[Loader] Boot device number: %d\n", (int)bd->bd_bootdev);

  CpuModel = bd->bd_cpumodel;
  BootDev = bd->bd_bootdev;
  ExcVecBase = bd->bd_vbr;

  {
    short i;

    for (i = 0; i < bd->bd_nregions; i++) {
      MemRegionT *mr = &bd->bd_region[i];
      uintptr_t lower = mr->mr_lower ? mr->mr_lower : 1;
      AddMemory((void *)lower, mr->mr_upper - lower, mr->mr_attr);
    }
  }

#ifndef AMIGAOS
  Log("[Loader] Executable file segments:\n");
  {
    HunkT *hunk = bd->bd_hunk;
    do {
      Log("[Loader] * $%08x - $%08lx\n",
          (u_int)hunk->data, (u_int)hunk->data + hunk->size - sizeof(HunkT));
      hunk = hunk->next;
    } while (hunk);
  }

  Log("[Loader] Setup shared hunks.\n");
  SetupSharedHunks(bd->bd_hunk);
#endif

  SetupExceptionVector(bd);
  SetupInterruptVector();

  /* CIA-A & CIA-B: Stop timers and return to default settings. */
  ciaa->ciacra = 0;
  ciaa->ciacrb = 0;
  /* unselect all drives */
  ciab->ciacra = CIAB_DSKSEL3|CIAB_DSKSEL2|CIAB_DSKSEL1|CIAB_DSKSEL0;
  ciab->ciacrb = 0;

  /* CIA-A & CIA-B: Clear pending interrupts. */
  (void)ciaa->_ciaicr;
  (void)ciab->_ciaicr;

  /* CIA-A & CIA-B: Disable all interrupts. */
  ciaa->_ciaicr = CIAICRF_ALL;
  ciab->_ciaicr = CIAICRF_ALL;

  /* Enable master bit in DMACON and INTENA */
  EnableDMA(DMAF_MASTER);
  EnableINT(INTF_INTEN);

  /* Lower interrupt priority level to nominal. */
  SetIPL(IPL_NONE);

#ifdef MULTITASK
  /* Main task wraps current boot stack, idle task executes STOP/WAIT loop. */
  TaskInit(CurrentTask, "main", bd->bd_stkbot, bd->bd_stksz);
  TaskInit(&IdleTask, "idle", IdleTaskStack, sizeof(IdleTaskStack));
  TaskRun(&IdleTask, 3, IdleTaskLoop, NULL);
#endif
  CallFuncList(&__INIT_LIST__);

  {
    __unused int retval = main();
    Log("[Loader] main() returned %d.\n", retval);
  }

  CallFuncList(&__EXIT_LIST__);

  Log("[Loader] Shutdown complete!\n");
}
