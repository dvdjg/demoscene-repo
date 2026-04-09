/*
 * Empty — minimal template effect: empty Init/Kill, animates COLOR0, optional asm hook.
 *
 * Each frame bumps COLOR0 via custom->color[0] (direct register poke). OptimizedFunction()
 * is an external asm microbenchmark hook. TaskWaitVBlank syncs to 50 Hz.
 *
 * HRM: https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 */
/*
 * Empty — Efecto plantilla: no dibuja nada, solo incrementa el color de fondo y llama
 * a OptimizedFunction() (para medir o probar código asm). Útil como base para nuevos
 * efectos o para comprobar el sistema Load/Init/Kill/Render/VBlank.
 */
#include <effect.h>

/* Search following header files for useful procedures. */
#include <custom.h>     /* custom registers definition and misc functions */   
#include <blitter.h>    /* blitter handling routines */
#include <color.h>      /* operations on RGB colors */
#include <copper.h>     /* copper list construction */
#include <bitmap.h>     /* bitmap structure */
#include <palette.h>    /* palette structure */
#include <pixmap.h>     /* pixel map (chunky) structure */
#include <sprite.h>     /* sprite structure and copper list helpers */
#include <system/interrupt.h> /* register & unregister an interrupt handler */
#include <system/memory.h>    /* dynamic memory allocator */

static void Init(void) {
  /* Intentionally empty: placeholder effect needs no resource allocation. */
}

static void Kill(void) {
  /* Intentionally empty: nothing to free in this minimal template. */
}

/* Optional asm/C optimized hook used for microbenchmarks or experiments. */
extern void OptimizedFunction(void);

/* Render — minimal frame loop:
 * 1) animate COLOR0 directly,
 * 2) call optimized hook,
 * 3) sync to vblank. */
static void Render(void) {
  /* Persistent counter so color cycles every frame. */
  static u_short c = 0;
  custom->color[0] = c++;
  OptimizedFunction();
  TaskWaitVBlank();
}

/* Standard effect wiring with only Init/Kill/Render callbacks. */
EFFECT(Empty, NULL, NULL, Init, Kill, Render, NULL);
