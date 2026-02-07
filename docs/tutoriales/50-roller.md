# Tutorial: Roller (Rodillo)

## Objetivo

Efecto de **rodillo**: una imagen o patrón que parece enrollarse como un **rodillo** o **cilindro** que gira. Visualmente suele ser un **scroll horizontal** con **deformación vertical**: las líneas no se desplazan todas igual, sino que su posición Y (o el contenido) depende de la “rotación” del cilindro (por ejemplo seno o lineal en X). Se puede hacer con **copper** (cambiar bplpt por línea para que cada línea lea de una fila distinta del buffer) o con **blitter** (copiar franjas con distintos offsets).

## Archivos clave

- `effects/roller/roller.c` — Init (bitmap(s), buffer de textura o imagen, copper), Render (actualiza offset o ángulo del rodillo, actualiza copper list o copia con blitter).
- Si usa copper por línea: lista que en cada línea pone bplpt a una fila diferente del bitmap.

## Flujo

1. **Init**: Crea bitmap de pantalla y, si hace falta, un buffer con la imagen o patrón que se “enrolla”. Precalcula una tabla con el **desplazamiento Y** (o índice de fila fuente) para cada línea de pantalla según el “ángulo” del rodillo (por ejemplo row_source = (y + phase) % height). Configura copper: si el efecto es por línea, la copper list tiene WAIT + bplpt (o BPL1PT) por cada línea apuntando a la fila correcta del buffer. Activa DMA.
2. **Render**: Actualiza **phase** (o ángulo) del rodillo según el tiempo. Si la copper se genera en software: reconstruye la parte de la copper que cambia (por ejemplo los bplpt por línea) según la nueva phase. Si se usa blitter: copia franjas horizontales del buffer al bitmap de pantalla con el offset correcto por franja. TaskWaitVBlank.
3. **Kill**: Libera bitmap(s) y copper.

## Mecanismos de hardware que lo hacen posible

- **Copper**: Puede **cambiar bplpt en cada línea** (WAIT y luego MOVEM bplpt, direccion). Así cada línea de pantalla puede leer de una **fila distinta** del mismo bitmap (o de un buffer). Eso da el efecto de “rodillo”: la imagen parece enrollada porque la “fila” que se muestra en Y no es Y, sino (Y + phase) mod height. El copper hace esto en paralelo a la CPU y sin coste de blitter.
- **Blitter**: Alternativa: para cada “banda” de líneas, **BlitterCopy** desde el buffer (con origen Y = (y_band + phase) % height) al bitmap de pantalla. Varias copias por frame; más uso de blitter pero la copper list puede ser fija.
- **CPU**: Calcula **phase** y, si la copper se genera dinámicamente, escribe la nueva lista (bplpt por línea) en chip RAM. También puede calcular los offsets de copia para el blitter.
- **DMA de raster**: Muestra el bitmap; si bplpt cambia por línea, el raster sigue la lista de la copper y muestra el efecto de rodillo directamente.
- **Chip RAM**: Bitmap y buffer en chip; la copper apunta a direcciones en chip.

## Técnicas y trucos

- **Tabla de bplpt por línea**: Para cada línea de pantalla, la dirección del primer palabra del bitmap es: base + (row_index * bytesPerRow). row_index = (y + phase) % texture_height. Precalcular esta tabla o calcularla cada frame y escribir en la copper list.
- **Phase**: Un contador que se incrementa cada frame (o cada N frames). phase mod texture_height da la “rotación” del rodillo. Si texture_height = 256, phase de 0 a 255 da una vuelta completa.
- **Suavizado**: Si la textura tiene menos filas que las líneas de pantalla, varias líneas comparten la misma fila fuente; el efecto sigue siendo correcto (repetición de la textura).

## Conceptos para principiantes

- **bplpt por línea** — La copper puede cargar un nuevo puntero de bitplane (bplpt) en cada línea de pantalla. Así la misma pantalla “lee” de distintas zonas de memoria según la línea, dando efectos de deformación o rodillo.
- **Phase** — Valor (ángulo o índice) que avanza con el tiempo y controla cuánto “giro” tiene el rodillo. Se usa para indexar qué fila del buffer corresponde a cada línea de pantalla.
- **Modulo (mod)** — (y + phase) % height asegura que el índice de fila siempre esté entre 0 y height-1, dando la sensación de un cilindro que se repite.
