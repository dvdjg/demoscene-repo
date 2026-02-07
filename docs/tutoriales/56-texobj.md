# Tutorial: TexObj (Objeto texturizado)

## Objetivo

Mostrar un **objeto 3D texturizado**: un polígono o malla 3D cuya superficie se rellena con una **textura** (imagen 2D) en lugar de un color plano. En Amiga suele hacerse con **mapeo UV** o **proyección de textura**: para cada pixel (o para cada scanline) del polígono proyectado se calculan coordenadas (u,v) en la textura y se toma el color de la textura en ese punto. El **blitter** puede hacer **copias con modulo** para simular textura por bandas; la **CPU** hace el cálculo de (u,v) y el sampleo si es por pixel.

## Archivos clave

- `effects/texobj/texobj.c` — Init (bitmap, textura en chip RAM, copper, geometría del objeto), Render (proyección 3D, por cada scanline del polígono: interpolar u,v, copiar línea de textura con BlitterCopy o muestrear en CPU).
- Textura: `data/` o generada (patrón).

## Flujo

1. **Init**: Carga o genera la **textura** (bitmap en chip RAM). Crea bitmap de pantalla y copper. Define el objeto (quad, triángulo o malla) con coordenadas 3D y **UV** (u,v por vértice). LoadColors (paleta de la textura). Activa DMA.
2. **Render**: Rotación/proyección del objeto. Para cada **cara** (triángulo o quad): proyectar vértices a 2D; tener (u,v) en cada vértice. Para cada **línea Y** dentro del polígono: interpolar u_left, u_right, v_left, v_right; para cada x entre x_left y x_right interpolar (u,v), **muestrear la textura** en (u,v) y escribir el píxel (o **BlitterCopy** una franja de la textura con el offset y escalado adecuados). Orden de caras por Z. TaskWaitVBlank.
3. **Kill**: Libera textura, bitmap y copper.

## Mecanismos de hardware que lo hacen posible

- **Blitter**: Para **textura por scanline**: en cada línea Y del polígono se calcula qué franja de la textura (qué fila v, y qué rango de u) corresponde. **BlitterCopy** puede copiar esa franja desde la textura al destino con **modulo** y **tamaño** correctos. Si la textura se repite (wrap), el modulo de la textura hace que al llegar al borde se lea del otro lado. El blitter hace la copia muy rápido; la CPU solo calcula origen y tamaño por línea.
- **CPU**: **Proyección 3D**, **interpolación de (u,v)** a lo largo de las aristas y por scanline (u = u_left + (u_right - u_left) * (x - x_left) / (x_right - x_left)). Si el relleno es por pixel, la CPU lee la textura y escribe en el bitmap (más lento pero flexible). Cálculo de **orden de caras** (painter’s algorithm).
- **Copper**: bplpt para pantalla y, si la textura es un bitmap aparte, el blitter la lee desde su dirección en chip. LoadColors para la paleta de la textura.
- **DMA de raster**: Muestra el bitmap de pantalla (ya relleno con la textura).
- **Chip RAM**: **Textura y bitmap de pantalla en chip** para que el blitter pueda leer la textura y escribir en pantalla.

## Técnicas y trucos

- **Interpolación UV**: En un triángulo, (u,v) se interpolan linealmente en 2D pantalla. u_left(y) = u0 + (u1-u0)*(y-y0)/(y1-y0) en la arista izquierda; igual para la derecha. En la línea horizontal: u(x) = u_left + (u_right - u_left)*(x - x_left)/(x_right - x_left).
- **Textura por línea (blitter)**: Si la variación de v por línea es 1 (una línea de pantalla = una línea de textura), se puede hacer una BlitterCopy por línea: origen = texture + v*tex_width, destino = screen + y*screen_width. Si hay escalado, el blitter puede usar diferente ancho de origen/destino (zoom).
- **Paleta**: La textura usa índices de color (por ejemplo 4 bitplanes = 16 colores). La misma paleta se usa en LoadColors para que los índices se vean correctos.

## Conceptos para principiantes

- **UV (mapeo)** — Coordenadas (u,v) en el cuadrado [0,1]x[0,1] que indican “qué punto de la textura” corresponde a cada vértice (o a cada píxel) del polígono. u=0,v=0 = esquina; u=1,v=1 = esquina opuesta.
- **Interpolación** — Calcular un valor en un punto intermedio a partir de los valores en los vértices. En un triángulo, (u,v) en un píxel interior es un promedio ponderado de (u,v) en los tres vértices.
- **BlitterCopy con modulo** — El blitter tiene registro de “modulo” para origen y destino (cuántos bytes saltar al pasar a la siguiente línea). Así se puede copiar desde una textura de ancho distinto al de la pantalla.
