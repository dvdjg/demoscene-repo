/*
 * Logging, assertions, panic — debug macros for demos.
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#ifndef __DEBUG_H__
#define __DEBUG_H__

/* When simulator is configured to enter debugger on illegal instructions,
 * this macro can be used to set breakpoints in your code. */
#define BREAK() { asm volatile("illegal"); }

/* Halt the processor by masking all interrupts and waiting for NMI. */
#define HALT() { asm volatile("stop #0x2700"); }

/* Invoke CrashHandler in order to display diagnostic screen. */
#define CRASH() { asm volatile("trap #15"); }

#include <system/debug.h>

#endif
