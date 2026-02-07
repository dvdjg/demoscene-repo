# Tutorial: Sea-anemone (Anémona)

## Objetivo

Efecto **anémona**: figura orgánica que recuerda una **anémona de mar** (tentáculos o rayos que se mueven desde un centro). Suele hacerse con **líneas** o **bandas** que parten de un punto central y se animan (ondulación, longitud o ángulo variable). Se dibuja con **CpuLine** o **BlitterLine** para cada “tentáculo”, o con **BlitterFill** para bandas rellenas; la animación se controla con **seno/coseno** (tablas) para movimiento fluido.

## Archivos clave

- `effects/sea-anemone/sea-anemone.c` — Init (bitmap, copper, tablas sin/cos), Render (calcula extremos de cada rayo según tiempo, dibuja líneas o rellenos).
- Posible `data/` con parámetros de número de rayos, amplitud, etc.

## Flujo

1. **Init**: Crea bitmap y copper. Precalcula tabla de senos (y cosenos si hace falta). Define número de “tentáculos” (por ejemplo 16 o 32) y parámetros (longitud base, amplitud de onda). LoadColors. Activa DMA.
2. **Render**: Para cada rayo: ángulo = base_angle + tentacle_index * step. Longitud y/o offset de onda dependen del tiempo (sin(frame + offset)). Calcula (x2,y2) = centro + (cos(angle)*length, sin(angle)*length) o con ondulación (varios segmentos por rayo). Dibuja **línea** desde centro hasta (x2,y2) con **CpuLine** o **BlitterLine**; o dibuja una banda (triángulo o trapecio) rellena con BlitterFill. Limpia o no el bitmap para estela. TaskWaitVBlank.
3. **Kill**: Libera bitmap y copper.

## Mecanismos de hardware que lo hacen posible

- **Blitter**: **BlitterLine** dibuja cada “tentáculo” muy rápido (modo línea). Si se usan bandas rellenas, **BlitterFill** para cada banda entre dos rayos consecutivos. Todo en chip RAM.
- **CPU**: Cálculo de **ángulos** y **longitudes** con seno/coseno (tablas), actualización de **frame** o **phase**, y preparación de coordenadas para cada línea. La lógica del efecto es en CPU.
- **Copper**: bplpt y LoadColors. Se puede variar color por línea (LoadColors en copper) para dar degradado radial o por profundidad.
- **DMA de raster**: Muestra el bitmap.
- **Chip RAM**: Bitmap en chip.

## Técnicas y trucos

- **Ondulación**: length = base_length + amplitude * sin(frame * speed + tentacle_index). Así cada rayo tiene un “latido” desfasado. O variar el ángulo: angle = base + wave * sin(frame + i).
- **Varios segmentos por rayo**: En lugar de una línea recta, dibujar varios segmentos (x0,y0)->(x1,y1)->(x2,y2)... donde cada punto se calcula con una onda. Da efecto de tentáculo que se ondula.
- **Paleta**: Colores que recuerden el mar (azules, violetas) o anémona (rojos, naranjas). Color cycling suave puede dar sensación de “respiración”.

## Conceptos para principiantes

- **Coordenadas polares** — En lugar de (x,y) se usa (ángulo, radio). x = center_x + cos(angle)*r, y = center_y + sin(angle)*r. Muy útil para efectos radiales como rayos o tentáculos.
- **Tabla de senos** — Array con los valores de sin(0), sin(1), ... para evitar calcular seno en tiempo real. Se usa índice = (angle + phase) & 0xFF para leer sin(index).
- **BlitterLine** — Modo del blitter que dibuja una línea entre dos puntos. Ideal para muchos segmentos desde un mismo centro (como los rayos de la anémona).
