/*
 * Demoscene tutorial — documentation pointers (not required to #include).
 *
 * Use these URLs in per-file tutorial comments when introducing hardware or AmigaOS APIs:
 *
 * Amiga Hardware Reference Manual (HRM), 3rd ed. — Custom chips, Blitter, Copper, DMA, CIA:
 *   https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 *
 * HRM structured HTML mirror (unofficial; handy for chapter links):
 *   http://amigadev.elowar.com/read/
 *
 * Amiga ROM Kernel Reference Manual (RKM) — Exec, DOS, libraries/devices:
 *   https://archive.org/details/amiga-rom-kernel-reference-manual
 *
 * Abbreviations: OCS/ECS (chip generations), HRM, RKM, CIA (timers), DMA, CHIP vs FAST RAM.
 */
#ifndef __TUTORIAL_REFS_H__
#define __TUTORIAL_REFS_H__
/*
 * Documentation standard for this codebase (apply when editing .c / .h):
 * - Every global / static file-scope variable: purpose and lifetime.
 * - Every public macro: parameters, expansion idea, and C equivalent if non-obvious.
 * - Every function: what it does, hardware vs CPU path, and why asm if used.
 * - Inline asm: always note approximate C equivalent and why asm wins on Amiga.
 * Full coverage is applied incrementally across modules; prefer this depth over
 * one-line headers alone.
 */
#endif
