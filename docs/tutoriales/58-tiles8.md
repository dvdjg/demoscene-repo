# Tutorial: Tiles8 (Tiles 8x8)

## Objetivo

Efecto de **tiles (baldosas)** de **8x8** píxeles: una pantalla o zona rellena con un **mapa de tiles** (qué tile va en cada celda) y una **tabla de tiles** (los bloques 8x8 de gráficos). Cada celda del mapa apunta a uno de los tiles; el **blitter** (o la CPU) **copia** el tile correspondiente desde la tabla al bitmap de pantalla. Muy usado en juegos y fondos.

## Archivos clave

- `effects/tiles8/tiles8.c` — Init (carga o genera tabla de tiles 8x8, mapa de pantalla, bitmap, copper), Render (recorre el mapa y para cada celda visible copia el tile con BlitterCopy).
- `data/`: posible imagen de tiles o mapa (array de índices de tile).

## Flujo

1. **Init**: Carga la **tabla de tiles** (por ejemplo 16 o 256 tiles de 8x8, en chip RAM). Carga o genera el **mapa** (array 2D: mapa[col][row] = índice de tile). Crea bitmap de pantalla y copper. LoadColors (paleta de los tiles). Activa DMA.
2. **Render**: Si hay scroll: actualiza offset (scroll_x, scroll_y). Para cada celda **visible** en pantalla: índice = mapa[col + scroll_cell_x][row + scroll_cell_y]. Origen del tile = tabla_tiles + índice * (8*bytesPerRow_tile) (para 8 líneas de 8 píxeles). **BlitterCopy** desde ese origen al destino (screen + col*8 + row*8*bytesPerRow_screen). Opcional: animación cambiando el mapa o la tabla de tiles. TaskWaitVBlank.
3. **Kill**: Libera tabla, mapa, bitmap y copper.

## Mecanismos de hardware que lo hacen posible

- **Blitter**: **BlitterCopy** es ideal para tiles: cada tile es un rectángulo pequeño (8x8). Se hace una copia por celda visible (o por tile único si se reutiliza). El blitter copia en chip RAM muy rápido; el **modulo** del origen es el ancho de la tabla de tiles (varios tiles por fila en la tabla). Así se puede tener una imagen grande (mapa) con muy pocos datos (solo índices) y una tabla de tiles.
- **CPU**: Calcula **qué celdas son visibles** (según scroll), **dirección de origen** = tabla + índice * tamaño_tile, y **dirección de destino** = pantalla + (col*8, row*8). Llama a BlitterCopy por cada tile. Si hay scroll sub-pixel, el origen puede tener offset en X/Y (y el blitter copia con ese offset).
- **Copper**: bplpt y LoadColors. La pantalla es el bitmap relleno con los tiles.
- **DMA de raster**: Muestra el bitmap.
- **Chip RAM**: **Tabla de tiles y bitmap de pantalla en chip** para que el blitter pueda leer y escribir.

## Técnicas y trucos

- **Organización de la tabla**: Tiles en filas: tile 0 en (0,0), tile 1 en (8,0), ... tile 31 en (0, 8). Origen = tabla + (index % tiles_per_row)*8 + (index / tiles_per_row)*8*bytesPerRow. O todos los tiles en una columna (más fácil: origen = tabla + index * 8*bytesPerRow si cada tile es 8 líneas).
- **Scroll**: scroll_x, scroll_y en celdas; las celdas visibles son desde (scroll_x, scroll_y) hasta (scroll_x + screen_tiles_x, …). Scroll sub-pixel: offset en píxeles dentro del primer tile; el blitter puede copiar con source X/Y no alineado (copiando un poco menos a un lado).
- **Animación**: Cambiar el mapa cada frame (agua, lava) o cambiar la tabla de tiles (frames de animación del mismo tile). Así los mismos índices muestran otro gráfico.

## Conceptos para principiantes

- **Tile (baldosa)** — Bloque fijo de píxeles (ej. 8x8) que se repite o se combina con otros para formar la pantalla. El “mapa” dice qué tile va en cada posición; la “tabla de tiles” contiene los gráficos de cada tile.
- **Mapa de tiles** — Array 2D donde cada entrada es un índice (0, 1, 2, …) que indica qué tile dibujar en esa celda. Así una pantalla de 40x25 celdas son solo 1000 bytes (si cada índice es 1 byte).
- **BlitterCopy** — Copia rectangular de una zona de memoria a otra. Para tiles: origen = un rectángulo 8x8 en la tabla, destino = posición (col*8, row*8) en pantalla.
