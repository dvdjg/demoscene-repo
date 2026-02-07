# Tutorial: TileZoomer (Zoom de tiles)

## Objetivo

Efecto de **zoom sobre un mapa de tiles**: la vista del mapa de tiles se **aleja o acerca** (zoom in/out), mostrando más o menos celdas y con **escalado** de cada tile. Se combina el sistema de tiles (mapa + tabla) con **reescalado**: cada tile se dibuja más grande o más pequeño. Puede hacerse con **blitter** (varias copias por tile con repetición de líneas/píxeles para zoom in, o copia reducida para zoom out) o con **CPU** (muestreo del tile al tamaño destino).

## Archivos clave

- `effects/tilezoomer/tilezoomer.c` — Init (tabla de tiles, mapa, bitmap, copper), Render (calcula zoom level, para cada celda visible calcula tamaño en pantalla y posición; copia/escala cada tile con blitter o CPU).
- Posible lógica de “cuántas celdas caben” según zoom (zoom in = menos celdas, tiles más grandes).

## Flujo

1. **Init**: Carga tabla de tiles y mapa. Crea bitmap y copper. LoadColors. Define zoom inicial (por ejemplo 1.0 = 1 pixel del tile = 1 pixel pantalla). Activa DMA.
2. **Render**: Actualiza **zoom** (por ejemplo zoom += 0.02 cada frame). Calcula **cuántas celdas** caben en pantalla: si cada tile se dibuja de tamaño (tile_size * zoom), las celdas visibles son (pantalla_ancho / (tile_w*zoom)), etc. Para cada celda visible: **origen** = tile en la tabla (siempre mismo tamaño, ej. 16x16). **Destino** = (col * tile_w * zoom, row * tile_h * zoom) con **tamaño** (tile_w*zoom, tile_h*zoom). Si zoom >= 1: BlitterCopy con repetición (copiar cada línea del tile varias veces, cada palabra varias veces). Si zoom < 1: copia reducida (saltar líneas/píxeles) o muestreo en CPU. TaskWaitVBlank.
3. **Kill**: Libera recursos.

## Mecanismos de hardware que lo hacen posible

- **Blitter**: Para **zoom in** (escala > 1): no hay “escalado” directo; se simula **repitiendo**: cada línea del tile se copia N veces (N = zoom), y cada palabra o cada píxel se repite. Varias BlitterCopy o una con destino de mayor altura (copiar la misma línea de origen varias veces con distintos dest_y). Para **zoom out** (escala < 1): copiar solo 1 de cada N líneas y 1 de cada N palabras del tile; el blitter puede hacerlo con modulo y tamaño de origen mayor que destino (leer 2 palabras, escribir 1). La CPU calcula los parámetros; el blitter hace la copia/escala.
- **CPU**: Cálculo de **zoom**, **número de celdas visibles**, **posición y tamaño** de cada tile en pantalla. Si el escalado es complejo (no potencia de 2), puede hacerse en CPU: para cada píxel destino, calcular (sx, sy) en el tile y leer ese píxel.
- **Copper**: bplpt, LoadColors.
- **DMA de raster**: Muestra el bitmap.
- **Chip RAM**: Tabla y pantalla en chip.

## Técnicas y trucos

- **Zoom in con blitter**: Por cada línea del tile: BlitterCopy de esa línea (origen 1 línea, destino N líneas con mismo contenido). Repetir para cada línea del tile. O: destino con modulo 0 y varias veces la misma línea (complicado); más simple: N copias de la misma línea a dest_y, dest_y+1, … dest_y+N-1.
- **Zoom out**: Origen = tile completo; destino = rectángulo más pequeño. Copiar leyendo cada 2ª (o cada Kª) línea y cada 2ª palabra. Modulo de origen = 2*ancho_tile para “saltar” una línea; ancho de copia = mitad. No todos los blitters permiten esto fácilmente; a veces se hace en CPU o con tiles pre-escalados en la tabla.
- **Centrado**: offset_x = (screen_width - num_cols * tile_w * zoom) / 2; igual para Y. Así el mapa queda centrado al hacer zoom.

## Conceptos para principiantes

- **Zoom in/out** — Ampliar (ver menos celdas, cada tile más grande) o reducir (ver más celdas, cada tile más pequeño). En Amiga el blitter no tiene “escala” directa; se simula repitiendo o saltando píxeles/líneas.
- **Celdas visibles** — Depende del zoom: a mayor zoom, cada tile ocupa más píxeles, así que caben menos celdas en pantalla. num_cols = screen_width / (tile_width * zoom).
- **Repetición de líneas** — Para “estirar” verticalmente: la misma línea de origen se escribe en varias líneas de destino. El blitter puede hacerlo con varias copias o con configuración especial de destino.
