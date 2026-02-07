# Tutorial: UVMap (Mapeo UV)

## Objetivo

Demostrar **mapeo UV** sobre un objeto 3D: una **textura** 2D se “envuelve” sobre la superficie 3D usando coordenadas **(u,v)** en cada vértice. Cada cara (triángulo o quad) se rellena muestreando la textura según (u,v) interpoladas. En Amiga se hace con **CPU** (interpolación de u,v por scanline, lectura de la textura y escritura en el bitmap) o con **blitter** (copiar franjas de textura con el offset correcto por línea). Es la base de **TexObj** y efectos de objeto texturizado.

## Archivos clave

- `effects/uvmap/uvmap.c` — Init (textura en chip RAM, geometría con (u,v) por vértice, bitmap, copper), Render (proyección 3D, por cada cara: relleno con interpolación UV y sampleo de textura).
- Textura: `data/` o generada.

## Flujo

1. **Init**: Carga o genera la **textura** (bitmap en chip). Define el **objeto** (por ejemplo un quad o un cubo) con vértices en 3D y **(u,v)** para cada vértice (ej. 0,0 / 1,0 / 1,1 / 0,1 para un quad). Crea bitmap de pantalla y copper. LoadColors (paleta de la textura). Activa DMA.
2. **Render**: **Rotación/proyección** del objeto. Para cada **cara**: proyectar vértices a 2D; tener (u,v) en cada vértice. **Ordenar** caras por Z. Para cada cara: para cada **línea Y** dentro del polígono 2D, interpolar **u_left, u_right, v_left, v_right**; para cada x (o por bloques) interpolar (u,v), **leer textura[u,v]** y escribir píxel en (x,y). Si se usa blitter: por cada línea Y, BlitterCopy una franja de la textura (fila v, desde u_left hasta u_right) al destino. TaskWaitVBlank.
3. **Kill**: Libera textura, bitmap y copper.

## Mecanismos de hardware que lo hacen posible

- **Blitter**: **BlitterCopy** puede copiar una **franja horizontal** de la textura (una línea de píxeles, o un rectángulo) al bitmap de pantalla. La **CPU** calcula para cada línea Y del polígono qué (u,v) corresponden a los extremos izquierdo y derecho; el blitter copia la fila v (o la parte entre u_left y u_right) al destino. El **modulo** de la textura permite que u wrap (textura que se repite). Sin blitter, la CPU tendría que escribir cada píxel (más lento).
- **CPU**: **Proyección 3D**, **interpolación de (u,v)** a lo largo de las aristas (u = u0 + t*(u1-u0), t en [0,1]) y por **scanline** (u(x) = u_left + (u_right-u_left)*(x-x_left)/(x_right-x_left)). Cálculo de **orden de caras** (painter’s algorithm). Si el relleno es por pixel, la CPU lee la textura y escribe en el bitmap.
- **Copper**: bplpt, LoadColors. La textura está en chip para que el blitter la lea.
- **DMA de raster**: Muestra el bitmap ya relleno.
- **Chip RAM**: Textura y bitmap en chip.

## Técnicas y trucos

- **UV por vértice**: En un quad, asignar (0,0), (1,0), (1,1), (0,1) a las cuatro esquinas. Así la textura cubre exactamente la cara. En un triángulo, (0,0), (1,0), (0.5,1) por ejemplo.
- **Interpolación lineal**: En un triángulo, (u,v) en un punto interior = combinación lineal de (u,v) de los 3 vértices (pesos por área o por coordenadas baricéntricas). Por scanline es más simple: interpolar en las dos aristas que cruzan esa Y, luego interpolar en horizontal.
- **Wrap**: u = u % 1 o (u & (tex_width-1)) si la textura es potencia de 2. Así la textura se repite.
- **Objeto simple**: Empezar con un solo quad (rectángulo 3D) para probar el pipeline UV; luego pasar a cubo o esfera.

## Conceptos para principiantes

- **UV** — Coordenadas en el espacio de la textura. u suele ser horizontal (0 = izquierda, 1 = derecha), v vertical (0 = arriba, 1 = abajo). Cada vértice del polígono tiene su (u,v) que indica “qué punto de la textura va aquí”.
- **Interpolación** — Calcular (u,v) en un píxel interior del polígono a partir de los (u,v) de los vértices. Se hace linealmente a lo largo de las aristas y luego a lo largo de la línea horizontal (scanline).
- **Mapeo UV** — Asignar (u,v) a cada vértice de la malla 3D. Al dibujar, cada píxel de la cara toma el color de la textura en la (u,v) interpolada. Es el estándar para texturizar en 3D.
