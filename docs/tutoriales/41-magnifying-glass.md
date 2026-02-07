# Tutorial: Magnifying-glass (Lupa)

## Objetivo

Efecto de **lupa**: una zona de la pantalla se ve “ampliada” como si hubiera una lente encima. Se suele hacer tomando una región del **buffer de pantalla** (o de una textura) y dibujándola **escalada** en otra zona; el escalado puede ser con **BlitterCopy** (zoom) o con **CPU** (sampleo por coordenadas).

## Archivos clave

- `effects/magnifying-glass/magnifying-glass.c` — Init (bitmap, copper), Render (copia/reescala la región “debajo de la lupa” a la posición de la lupa).
- Si hay máscara circular: `data/` con shape de círculo o rutina que dibuja solo dentro del círculo.

## Flujo

1. **Init**: Crea bitmap (pantalla principal). Carga o genera la imagen “base” que se verá (puede ser otra pantalla o el mismo bitmap). Configura copper. Si la lupa es circular, prepara máscara o rutina de dibujo circular.
2. **Render**: Calcula la región fuente (por ejemplo centro de la lupa en coordenadas de mundo, tamaño de la lupa). **Copia** esa región al destino con **escalado**: BlitterCopy con modificación de paso (modulo) para “estirar” (zoom in), o un bucle que para cada píxel de la lupa lee el píxel correspondiente en la fuente (muestreo con coordenadas reales). Si hay máscara circular, se aplica AND con la máscara o se dibuja solo dentro del círculo. Actualiza bplpt, TaskWaitVBlank.
3. **Kill**: Libera bitmap y copper.

## Mecanismos de hardware que lo hacen posible

- **Blitter**: **BlitterCopy** puede hacer zoom si se usa **diferente paso (modulo)** en origen y destino: por ejemplo, leer cada 2 palabras en origen y escribir 1 en destino = zoom out; leer 1 y escribir 2 = zoom in. Para una lupa “2x” se puede copiar la misma línea dos veces (repitiendo filas) y en horizontal duplicar palabras. El blitter hace la copia en chip RAM muy rápido.
- **CPU**: Si el escalado es arbitrario (no potencia de 2), se suele hacer en CPU: para cada (x,y) en la zona de la lupa se calcula (sx, sy) en la imagen fuente (sx = x/zoom + cx, sy = y/zoom + cy) y se lee ese píxel y se escribe en (x,y). Más lento pero flexible.
- **Copper**: Muestra el bitmap; no hay efecto especial por línea para la lupa (la lupa es solo contenido del bitmap).
- **DMA de raster**: Muestra el resultado.
- **Chip RAM**: Origen y destino en chip para que el blitter pueda operar.

## Técnicas y trucos

- **Zoom con blitter**: Para 2x: copiar cada fila dos veces (destino avanza 2*bytesPerRow por cada fila de origen). Para 4x: repetir 4 veces. El “centro” de la lupa se controla eligiendo qué región del bitmap fuente se copia.
- **Máscara circular**: Tener un bitmap de máscara (1 dentro del círculo, 0 fuera). Después de copiar la zona ampliada, hacer BlitterCopy con la máscara (modo minterm que combine fuente AND máscara) para que solo se vea dentro del círculo. O dibujar el borde de la lupa encima (círculo) para dar sensación de cristal.
- **Suavizado**: En CPU se puede hacer bilinear (promedio de 4 píxeles vecinos) al muestrear; en blitter puro no hay filtro, solo repetición de píxeles.

## Conceptos para principiantes

- **Zoom (escalado)** — Mostrar una región de la imagen más grande (zoom in) o más pequeña (zoom out). En hardware Amiga el blitter no tiene “escalado” directo; se simula repitiendo o saltando filas/píxeles.
- **Máscara** — Bitmap auxiliar que indica “dónde” aplicar un dibujo: 1 = sí, 0 = no. Al combinar con AND, solo quedan los píxeles donde la máscara es 1.
- **Muestreo** — Obtener el color/píxel en coordenadas (sx, sy) de una imagen para ponerlo en (x, y). Si sx, sy no son enteros, se usa interpolación (vecino más próximo, bilinear, etc.).
