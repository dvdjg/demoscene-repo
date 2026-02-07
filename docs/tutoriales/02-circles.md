# Tutorial: Circles

## Objetivo

Dibujar **círculos concéntricos** que crecen desde el centro de la pantalla, en blanco sobre fondo negro, usando un solo bitplane.

## Archivos clave

- `effects/circles/circles.c` — Init, Kill, Render y llamada a `Circle()`.
- `include/circle.h` — Declaración de `Circle()`.
- `lib/libgfx/Circle.c` — Implementación del algoritmo de círculo (típicamente Bresenham o similar).

## Flujo

1. **Init**: crea un bitmap 320×256×1, configura playfield en MODE_LORES con 1 plano, pone COLOR0=negro y COLOR1=blanco, construye la copper list con `CopSetupBitplanes`, activa la lista y activa DMA de raster.
2. **Render**: en un bucle, para radios 2, 4, 6, … hasta casi la mitad de la altura, llama a `Circle(screen, 0, centerX, centerY, r)`. Luego `TaskWaitVBlank()`.
3. **Kill**: borra copper list y bitmap.

## Técnicas y trucos

- **Un solo bitplane**: con depth=1 solo hay dos “colores” (índices 0 y 1). El círculo se dibuja en el plano 0; el valor 1 del bit significa “píxel blanco”. Muy barato en memoria y en tiempo.
- **Circle(screen, plane, cx, cy, r)** — La librería dibuja el perímetro del círculo de radio `r` centrado en (cx, cy) en el plano indicado. Suele usar 8 simetrías (octantes) para calcular solo 1/8 del círculo y reflejar.
- **Sin doble buffer**: cada frame se redibuja todo; el bitmap se limpia implícitamente o no (depende de la implementación de Circle). Para un efecto “que crece” a 50 Hz, redibujar cada frame es aceptable.

## Conceptos para principiantes

- **MODE_LORES** — 320 píxeles lógicos de ancho; cada “píxel” ocupa un ciclo de DMA en lores.
- **CopSetupBitplanes(cp, screen, depth)** — Añade a la lista copper los MOVE que cargan los punteros de los planos de bits (bplpt) y el modulo. La Denise usa esos punteros para leer la imagen cada línea.
- **SetColor(índice, valor)** — Escribe en el registro COLORxx. Valor en 12 bits (0x000–0xfff): 4 bits rojo, 4 verde, 4 azul.
