/*
 * Minimal program entry for the demo framework (crt0.S).
 *
 * Purpose: this is the first code that runs after the OS or boot sector hands
 * control to our executable. It either jumps straight into Loader (bare-metal
 * trackmo) or, when AMIGAOS is defined, snapshots critical AmigaOS state
 * (SaveOS), runs Loader, then restores the OS (RestoreOS) so Workbench comes
 * back intact.
 *
 * Conceptual C equivalent (AMIGAOS path):
 *
 *   void start(void) {
 *     void *os_cookie = SaveOS();   // opaque handle in d0 -> passed in a0
 *     Loader(os_cookie);
 *     RestoreOS();
 *     return 0;                     // success to caller (CLI/Workbench)
 *   }
 *
 * Conceptual C equivalent (bare-metal path):
 *
 *   void start(void) {
 *     Loader();
 *     for (;;) { }                  // halt if Loader ever returns
 *   }
 *
 * Why assembly instead of C:
 * - The linker script must bind the real entry symbol (`start`) to the first
 *   instruction; a few instructions cost nothing and avoid pulling in generic
 *   CRT init (ctors, BSS clear done elsewhere, etc.).
 * - `SaveOS` returns a value in d0 that must reach `Loader` in a0 without
 *   spilling to memory — trivial in asm, more ceremony in C unless you wrap
 *   Loader’s prototype.
 *
 * AMIGAOS path detail: `SaveOS` preserves chipset/OS state; `Loader` receives
 * the cookie in a0 so teardown can be symmetric. The final `rts` returns to
 * whoever launched the program (shell/WB) with success in d0.
 *
 * Non-AMIGAOS: `jra .` is an infinite branch-to-self. A trackloader normally
 * never returns; if Loader returns, we stop dead rather than executing garbage.
 *
 * Hardware / OS docs (Amiga OCS/ECS, Exec):
 *   HRM: https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 *   RKM: https://archive.org/details/amiga-rom-kernel-reference-manual
 */
#include <asm.h>
#include <config.h>

ENTRY(start)
#ifdef AMIGAOS
        /* Snapshot Exec/custom state so we can restore video, DMA, CIA, etc. */
        jsr     _L(SaveOS)
        /* Loader expects the opaque OS cookie in a0 (not on the stack). */
        move.l  d0, a0
        jsr     _L(Loader)
        /* Undo SaveOS: reopen OS resources, restore view, timers, caches. */
        jsr     _L(RestoreOS)
        /* Report success to the program’s parent (d0 = 0 is conventional). */
        moveq.l #0, d0
        rts
#else
        /* Bare-metal: bring up the kernel/effect runtime; no OS to restore. */
        jsr     _L(Loader)
        /* If we ever fall through, halt instead of executing random bytes. */
        jra     .
#endif
END(start)

# vim: ft=gas:ts=8:sw=8:noet
