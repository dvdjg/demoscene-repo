# Bajo nivel: runtime y sistema

## Bucle principal

El framework actual usa el ciclo `EffectT`: `Load` → `Init` → `Render` (por frame) → `Kill` → `UnLoad` (`include/effect.h`). El motor puede:

- Vivir **dentro** de un efecto (recomendado al principio), o
- Sustituir gradualmente la lógica de `Render` por un `GameLoop` interno que encaje con `TaskWaitVBlank` y contadores de frame.

## Tiempo y vblank

- **PAL** típico 50 Hz: sincronización con `TaskWaitVBlank` o interrupción vertical.
- El motor de alto nivel debería expresar `dt` en segundos o en “ticks” enteros; la capa baja fija la correspondencia con `frameCount` / CIA.

Roadmap interno:

- Documentar si el juego usa **paso fijo** (acumulador de tiempo) o **un frame = un tick lógico**.
- Evitar asignación en el manejador de IRQ; solo señales y copias mínimas.

## Takeover y restauración

Equivalente conceptual a `system/amigaos.c`: guardar DMA, interrupciones, copper, CIA y restaurar al salir. El motor no debe duplicar esto si el binario ya corre bajo el loader del framework; sí debe **documentar** qué asume (por ejemplo, que el copper y el blitter quedan “en propiedad” del juego durante la escena).

## Entrada

Reutilizar `system/drivers/keyboard.c`, ratón, joystick según necesidad. La API de alto nivel debería entregar **estados por frame** (bordes, repetición) sin polling crudo en la lógica de juego.

## Multitarea y carga

Si `MULTITASK` está activo, las cargas en segundo plano pueden convivir con precálculos. El roadmap debe fijar qué subsistemas son **thread-safe** y cuáles solo se tocan desde el hilo principal del juego.

## Tests sugeridos

- **Host:** mock de tiempo (`GameLoop` con callback contado).
- **Target:** “no crash en N frames” + log por UAE trap si está disponible.

Anterior: [Hitos y fases](03-hitos-fases.md). Siguiente: [HAL y memoria](05-bajo-nivel-hal-memoria.md).
