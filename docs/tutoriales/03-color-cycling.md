# Tutorial: Color-cycling (Logo)

## Objetivo

Mostrar una **imagen estática** (logo) y animarla **solo cambiando la paleta**: los índices de color en el bitmap no cambian, pero los valores RGB de cada índice sí, creando la sensación de movimiento o “respiración” de color.

## Archivos clave

- `effects/color-cycling/color-cycling.c` — Init, Kill, Render y lógica de color cycling.
- `data/image.c` — Bitmap del logo (planos) y paleta base.
- `data/image-cycling.c` — Definición de qué registros rotar y en qué orden (ColorCyclingT).

## Flujo

1. **Init**: configura playfield con las dimensiones del logo, carga la paleta inicial con `LoadColors`, crea copper list con `CopSetupBitplanes` para el bitmap del logo, activa copper y DMA de raster.
2. **Render**: llama a `ColorCyclingStep(logo_cycling, ...)`, que avanza el “reloj” de cada grupo de ciclado y, cuando toca, rota los colores (cambia qué RGB tiene cada registro). Luego `TaskWaitVBlank()`.
3. **Kill**: para el copper y borra la lista.

## Técnicas y trucos

- **Animación sin tocar píxeles**: el bitmap no se modifica; solo se cambian los 16 (o 32) registros COLORxx. Muy barato en CPU y en ciclos de memoria.
- **ColorCyclingT**: para cada “rueda” de color se define: `rate` (cada cuántos “ticks” avanza un paso), `step` (acumulador), `ncolors` (cuántos colores en el ciclo), `cells` (qué registros COLORxx pertenecen a esta rueda) y `colors` (array de valores RGB en orden). Cada vez que `step >= rate`, se avanza el índice y se hace `SetColor(reg, colors[idx])`.
- **PAL_FRAME** — Factor para que el avance sea a ~50 Hz; `step += PAL_FRAME` cada frame y se compara con `rate` para decidir si rotar.

## Conceptos para principiantes

- **Paleta (color registers)** — En OCS/ECS hay 32 registros (COLOR00–COLOR31). Cada píxel en pantalla es un índice 0–31 (según depth); el color mostrado es el valor actual de ese registro. Cambiar el registro cambia todos los píxeles que usan ese índice.
- **LoadColors(palette, start)** — Carga varios colores consecutivos en los registros a partir de `start`. Suele usarse al inicio; durante el efecto se usa `SetColor` para actualizar registros concretos.
- **Copper solo para refresco** — Aquí el copper solo pone los punteros de bitplanes; la animación es 100 % cambio de paleta en CPU cada frame.
