# Tutorial: Rotator

## Objetivo

Efecto **rotador**: una imagen o patrón que **rota** en pantalla (como un disco o una rueda). Se suele hacer con **blitter** copiando la imagen desde un **buffer pre-rotado** (double buffer: se rota en uno y se muestra el otro) o con **copper** cambiando **bplpt** por línea para que cada línea lea de una franja distinta del buffer (efecto “rotación” por desplazamiento horizontal por línea). Alternativamente, **reescalado/rotación por software** (CPU) con muestreo del bitmap origen.

## Archivos clave

- `effects/rotator/rotator.c` — Init (bitmap de pantalla, buffer de imagen, copper), Render (actualiza ángulo, genera frame rotado con blitter o CPU, o actualiza copper con bplpt por línea).
- Posible uso de tabla de senos/cosenos para coordenadas de rotación.

## Flujo

1. **Init**: Crea bitmap de pantalla y buffer(es) con la imagen a rotar. Si la rotación es “por copper” (desplazamiento horizontal por línea): precalcula o genera la copper list que en cada línea pone bplpt (o BPL1PT) con el offset correcto para simular la inclinación. Si la rotación es real: prepara rutina de rotación (blitter con múltiples copias desplazadas, o CPU que para cada píxel destino calcula origen con cos/sin). LoadColors. Activa DMA.
2. **Render**: Actualiza **ángulo** de rotación. Si es copper: actualiza la parte de la copper que depende del ángulo (offsets por línea). Si es blitter/CPU: genera el frame rotado en el buffer de pantalla (muestreo desde la imagen origen con (x,y) rotados). Copia o muestra el resultado. TaskWaitVBlank.
3. **Kill**: Libera bitmaps y copper.

## Mecanismos de hardware que lo hacen posible

- **Copper**: Si el efecto es “pseudo-rotación” (imagen inclinada): **cambiar bplpt por línea** hace que cada línea lea desde un **desplazamiento horizontal** distinto del buffer. Eso da sensación de “esquina” o inclinación; no es una rotación completa pero es muy barata y no usa blitter para la rotación.
- **Blitter**: Para rotación real: una técnica es tener la imagen en un buffer y, para cada línea destino, **BlitterCopy** una franja del buffer con un **offset horizontal** que depende del ángulo y de la línea (offset = line * tan(angle) * bytesPerPixel o similar). Varias copias por frame. Otra: **blitter con fuente en modo “estrecho”** y destino con modulo para “inclinar”. El blitter hace las copias muy rápido en chip RAM.
- **CPU**: Cálculo del **ángulo** y de los **offsets** (tabla sin/cos). Si la rotación es por software: para cada (x,y) destino se calcula (sx,sy) origen con la matriz de rotación inversa y se lee el píxel; más lento pero rotación arbitraria.
- **DMA de raster**: Muestra el bitmap resultante.
- **Chip RAM**: Buffers en chip para blitter y para la copper.

## Técnicas y trucos

- **Rotación por líneas (copper)**: Cada línea lee desde base + (line * shift), donde shift depende del ángulo. shift = tan(angle) * bytesPerRow aproximado. La copper escribe bplpt para esa línea; el raster dibuja esa fila desde la nueva posición. Efecto de “paralelogramo” o disco inclinado.
- **Tabla de senos**: Precalcular sin/cos para 0..255 o 0..360 para no hacer cálculos trigonométricos cada frame. Ángulo = (frame * speed) & 0xFF.
- **Double buffer**: Rotar a un buffer y mostrar el otro; al siguiente frame se intercambian. Así no se ve “medio frame” mientras se dibuja.

## Conceptos para principiantes

- **Rotación 2D** — Dado un punto (x,y) y un ángulo a, el punto rotado es (x*cos(a)-y*sin(a), x*sin(a)+y*cos(a)). Para rotar una imagen entera se aplica la fórmula inversa a cada píxel destino para saber de qué píxel origen leer.
- **bplpt por línea** — La copper puede apuntar cada línea de pantalla a una fila distinta (o con offset) del bitmap. Muy útil para efectos de “inclinación” sin rotar píxel a píxel.
- **Tan(angle)** — La tangente del ángulo da cuánto se “desplaza” una línea respecto a la anterior en una rotación vista de lado. Por eso el offset por línea suele ser proporcional a tan(angle).
