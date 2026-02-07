# Tutorial: Lines

## Objetivo

Efecto basado en **muchas líneas**: puede ser un campo de líneas que se mueven, un starfield con líneas, o un dibujo abstracto. Las líneas se dibujan con **CpuLine** o **BlitterLine**; el código puede tener rutinas **optimizadas en asm** (CpuEdgeOpt.asm, CpuLineOpt.asm) para dibujar más rápido.

## Archivos clave

- `effects/lines/lines.c` — Init (bitmap, copper), Render (calcula extremos de las líneas, llama a CpuLine o BlitterLine para cada una).
- `effects/lines/CpuEdgeOpt.asm`, `CpuLineOpt.asm` — Rutinas en ensamblador que dibujan líneas (o bordes) de forma muy optimizada (menos ciclos por píxel).

## Flujo

1. **Init**: Crea bitmap y copper. Carga o genera las posiciones iniciales de las líneas (array de x1,y1,x2,y2 o parámetros que se usan para calcularlas). **CpuLineSetup**(screen, plane) o **BlitterLineSetup**(screen, plane, mode). Activa DMA (blitter si se usa). SetupPlayfield, LoadColors.
2. **Render**: Actualiza las posiciones de las líneas (por ejemplo rotación, desplazamiento, o nueva posición aleatoria). Para cada línea: **CpuLine(x1,y1,x2,y2)** o **BlitterLine(x1,y1,x2,y2)**. Si se usa asm optimizado, se llama a la rutina que recibe los parámetros en registros o en memoria. Limpia el bitmap al inicio del frame (o no, para efecto de estela). Actualiza bplpt, TaskWaitVBlank.
3. **Kill**: Libera bitmap y copper.

## Mecanismos de hardware que lo hacen posible

- **Blitter** (si se usa BlitterLine): Tiene **circuito dedicado para líneas** (modo línea): implementa Bresenham en hardware. Es mucho más rápido que dibujar en CPU píxel a píxel. Se configura una vez (bitmap, plano, modo OR/EOR) y luego cada BlitterLine solo envía los dos puntos; el blitter hace el resto.
- **CPU (68000)** (si se usa CpuLine o asm): **CpuLine** usa algoritmo de línea (Bresenham) en software: escribe cada píxel con MOVE.B o MOVE.W en el bitmap. Las versiones **CpuEdgeOpt** y **CpuLineOpt** en asm reducen el número de instrucciones por píxel (unrolling, direccionamiento más eficiente) para acercarse al rendimiento del blitter cuando hay muchas líneas pequeñas.
- **Copper**: Mantiene bplpt y paleta. La imagen es el bitmap con todas las líneas dibujadas.
- **DMA de raster**: Muestra el bitmap.
- **Chip RAM**: Bitmap en chip; el blitter solo escribe en chip, y el raster solo lee de chip.

## Técnicas y trucos

- **BlitterLine vs CpuLine**: BlitterLine es mejor cuando hay muchas líneas o líneas largas; la CPU queda libre. CpuLine (o asm) puede ser mejor si las líneas son muy cortas (pocos píxeles) porque el blitter tiene un coste de arranque por línea.
- **Asm optimizado**: CpuLineOpt.asm suele desenrollar el bucle de Bresenham y usar direccionamiento por registro (ej. A0 apuntando al bitmap, D0/D1 para x,y). Así se ahorran ciclos por píxel.
- **Modo EOR**: Si se usa BlitterLine en modo EOR, dibujar la misma línea dos veces la “borra”. Útil para animación sin borrar todo el bitmap (dibujar en EOR, mover, volver a dibujar en EOR en la posición antigua, dibujar en la nueva).
- **Recorte**: Si las líneas pueden salir de pantalla, usar ClipLine2D (lib2d) antes de dibujar para no escribir fuera del bitmap.

## Conceptos para principiantes

- **Bresenham** — Algoritmo para dibujar una línea entre dos puntos usando solo sumas y comparaciones (sin multiplicaciones ni divisiones). Genera la secuencia de píxeles que mejor aproximan la línea.
- **Modo OR / EOR** — Al dibujar en un plano: OR hace que el píxel quede en 1; EOR (XOR) invierte el píxel. EOR dos veces vuelve al estado anterior = “borrar” la línea sin borrar todo.
- **Rutina en asm** — Código en lenguaje ensamblador 68000 que se llama desde C. Se usa para las partes más críticas (bucles de dibujo) donde cada ciclo de CPU cuenta.
