# Tutorial: Tiles16 (Tiles 16x16)

## Objetivo

Igual que **Tiles8** pero con tiles de **16x16** píxeles. Un **mapa de tiles** indica qué tile va en cada celda; una **tabla de tiles** contiene los bloques 16x16. El **blitter** copia cada tile visible desde la tabla al bitmap de pantalla. Tiles más grandes permiten menos celdas y menos copias por frame, o más detalle por tile.

## Archivos clave

- `effects/tiles16/tiles16.c` — Init (tabla de tiles 16x16, mapa, bitmap, copper), Render (para cada celda visible, BlitterCopy del tile correspondiente).
- `data/`: tabla de tiles y/o mapa.

## Flujo

1. **Init**: Carga **tabla de tiles** (cada tile 16x16). Carga **mapa** (celdas = índices de tile). Crea bitmap y copper. LoadColors. Activa DMA.
2. **Render**: Actualiza scroll si hay. Para cada celda visible: índice = mapa[col][row]; origen = tabla + índice * (16 * bytesPerRow_tile o según organización). **BlitterCopy** 16x16 desde origen a destino (col*16, row*16). TaskWaitVBlank.
3. **Kill**: Libera recursos.

## Mecanismos de hardware que lo hacen posible

- **Blitter**: **BlitterCopy** para cada tile 16x16. Mismo principio que Tiles8: copia rectangular desde la tabla al destino. El tamaño 16x16 implica más palabras por tile (más tiempo de blitter por celda) pero menos celdas para llenar la misma pantalla.
- **CPU**: Cálculo de celdas visibles, dirección origen/destino, invocación de BlitterCopy. Scroll en celdas o sub-pixel.
- **Copper**: bplpt, LoadColors.
- **DMA de raster**: Muestra el bitmap.
- **Chip RAM**: Tabla y pantalla en chip.

## Técnicas y trucos

- **Tamaño de tabla**: 16x16 = 32 bytes por línea por plano; 6 planos = 192 bytes por tile. Organizar la tabla para que el **modulo** del blitter sea correcto (ej. tabla con 8 tiles por fila = 8*16 = 128 bytes por fila por plano).
- **Scroll**: Con tiles 16x16, scroll de 1 celda = 16 píxeles. Para scroll suave (pixel a pixel) hace falta o bien redibujar todo con offset de origen, o usar doble buffer y desplazar (más coste de blitter).
- **Reutilizar Tiles8**: La lógica es idéntica; solo cambian constantes TILE_W = 16, TILE_H = 16 y el cálculo de origen (índice * 16 * bytesPerRow o equivalente).

## Conceptos para principiantes

- **Tile 16x16** — Mismo concepto que 8x8 pero con bloques más grandes. Menos tiles en pantalla (ej. 20x12 celdas para 320x192) así que menos copias de blitter por frame.
- **Modulo (blitter)** — Bytes de salto entre líneas en origen/destino. En una tabla de tiles, el modulo del origen es el ancho total de la tabla (no el ancho del tile).
- **Scroll en celdas** — Scroll_x y scroll_y en unidades de celda (0, 1, 2…). La ventana visible son las celdas (scroll_x, scroll_y) hasta (scroll_x + ancho_visible, …). Muy barato: solo cambia qué índices del mapa se usan.
