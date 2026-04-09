/*
 * Exception vector initialization (CPU traps, not IRQs).
 *
 * Purpose: fill ExcVec[] with defaults (BadTrap) then specific handlers for bus
 * error, address error, illegal opcode, divide by zero, etc. TRAP #0 may map to
 * cooperative yield under MULTITASK; TRAP #15 routes to CrashHandler for a
 * controlled stop.
 *
 * ExcVecBase points at the CPU's vector table (0 on 68000, or VBR on 68010+).
 *
 * M68000 Family Programmer's Manual covers exception stack frames; HRM ties in
 * custom chip faults indirectly via bus errors on bad CHIP accesses.
 */
#include <config.h>
#include <system/boot.h>
#include <system/exception.h>
#include <system/trap.h>

/* Exception Vector Base: 0 for 68000, for 68010 and above read from VBR */
ExcVecT *ExcVecBase = (ExcVecT *)NULL;

/* SetupExceptionVector — fill CPU exception vectors with defaults and overrides.
 * TRAP #0 is reserved for scheduler yield in MULTITASK builds. */
void SetupExceptionVector(BootDataT *bd) {
  short i;

  /* Set up magic number and pointer to boot data for debugger tooling. */
  ExcVec[0] = (ExcSrvT)0x1EE7C0DE;
  ExcVec[1] = (ExcSrvT)bd->bd_hunk;

  /* Initialize M68k interrupt vector. */
  for (i = EXC_BUSERR; i <= EXC_LAST; i++)
    ExcVec[i] = BadTrap;

  /* Initialize exception handlers. */
  ExcVec[EXC_BUSERR] = BusErrTrap;
  ExcVec[EXC_ADDRERR] = AddrErrTrap;
  ExcVec[EXC_ILLEGAL] = IllegalTrap;
  ExcVec[EXC_ZERODIV] = ZeroDivTrap;
  ExcVec[EXC_CHK] = ChkInstTrap;
  ExcVec[EXC_TRAPV] = TrapvInstTrap;
  ExcVec[EXC_PRIV] = PrivInstTrap;
  ExcVec[EXC_TRACE] = TraceTrap;
  ExcVec[EXC_LINEA] = IllegalTrap;
  ExcVec[EXC_LINEF] = IllegalTrap;
  ExcVec[EXC_FMTERR] = FmtErrTrap;

  /* Intialize TRAP instruction handlers. */
#ifdef MULTITASK
  ExcVec[EXC_TRAP(0)] = YieldHandler;
#else
  ExcVec[EXC_TRAP(0)] = TrapInstTrap;
#endif

  for (i = EXC_TRAP(1); i < EXC_TRAP(15); i++)
    ExcVec[i] = TrapInstTrap;

  ExcVec[EXC_TRAP(15)] = CrashHandler;
}
