# Tutorial: Empty

## Objetivo

Un efecto **plantilla**: no dibuja nada útil, solo cambia el color de fondo cada frame y llama a una función “optimizada” (por ejemplo en asm). Sirve para comprobar que el sistema de efectos funciona y para medir tiempos de ejecución.

## Archivos clave

- `effects/empty/empty.c` — Todo el efecto.
- `include/effect.h` — Definición de `EFFECT()` y `EffectT`.
- `include/custom.h` — Puntero `custom` a los registros del hardware.

## Flujo

1. **Init**: vacío (no reserva memoria ni copper).
2. **Render**: incrementa un contador, escribe ese valor en `custom->color[0]` (registro COLOR00, color de fondo), llama a `OptimizedFunction()` y espera al siguiente VBlank con `TaskWaitVBlank()`.
3. **Kill**: vacío.

## Técnicas y trucos

- **Escribir en registros del hardware**: `custom->color[0] = c++;` — En Amiga, los colores son registros de 16 bits (4R+4G+4B). Modificar COLOR00 cambia el fondo al instante.
- **Plantilla mínima**: Load/UnLoad/Init/Kill pueden ser NULL o vacíos; solo Render es obligatorio para que “pase algo” cada frame.
- **OptimizedFunction()**: declarada `extern`; puede ser una rutina en asm (p. ej. `optimized.asm`) para medir ciclos o probar código sin tocar el resto del efecto.

## Conceptos para principiantes

- **custom** — Puntero a la estructura que mapea los registros del chip (color, blitter, copper, DMA). Escribir en `custom->reg` es escribir en el hardware.
- **VBlank** — Momento en que el haz de la CRT está en el blanking vertical; es cuando se suele actualizar punteros de bitplanes o paleta para evitar tearing. `TaskWaitVBlank()` sincroniza el efecto a 50 Hz (PAL).
- **EFFECT(Name, Load, UnLoad, Init, Kill, Render, VBlank)** — Macro que define la variable `NameEffect` con los siete callbacks. Cualquiera puede ser `NULL` si no se usa.
