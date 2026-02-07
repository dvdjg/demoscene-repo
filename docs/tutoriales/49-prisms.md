# Tutorial: Prisms (Prismas)

## Objetivo

Efecto de **prismas**: formas geométricas (triángulos o prismas 3D) que rotan o se mueven, a menudo con **caras de color sólido** o con **degradado** por cara. Se dibujan como **triángulos** o **quads** proyectados en 2D, rellenados con el blitter o con rutina de relleno de polígono.

## Archivos clave

- `effects/prisms/prisms.c` — Init (bitmap, copper, geometría de prismas), Render (rotación 3D, proyección, orden por Z, relleno de caras con BlitterFill o FillTriangle).
- Si usa lib3d: ProjectPoint3D, FillTriangle o equivalente.

## Flujo

1. **Init**: Crea bitmap y copper. Define los prismas (por ejemplo un triángulo 3D = 3 vértices; un prisma triangular = 2 triángulos + 3 rectángulos). Carga paleta. Activa DMA.
2. **Render**: Para cada prisma: aplica rotación (matriz o ángulos) a los vértices. **Proyecta** cada vértice a 2D (ProjectPoint3D). Ordena las caras por profundidad (Z medio o Z del vértice más cercano). Para cada cara (triángulo o quad): **rellena** con color sólido (BlitterFill para triángulo descompuesto en bandas horizontales, o rutina de relleno de polígono). Opcional: color o intensidad según Z (más oscuro atrás). TaskWaitVBlank.
3. **Kill**: Libera bitmap y copper.

## Mecanismos de hardware que lo hacen posible

- **Blitter**: **BlitterFill** rellena regiones (rectángulos o bandas horizontales). Un triángulo se puede rellenar dibujando una línea horizontal por cada Y entre y_min y y_max, con x_left y x_right calculados por interpolación entre las aristas. El blitter pinta cada línea muy rápido.
- **CPU**: **Rotación 3D** (multiplicar vértices por matriz o por ángulos), **proyección** (x’ = x/z * scale, y’ = y/z * scale), **ordenamiento** de caras (painter’s algorithm) y cálculo de los límites (x_left, x_right por línea) para el relleno.
- **Copper**: bplpt y LoadColors. Los colores de cada cara pueden ser fijos en la paleta o variar por línea (copper) para degradados.
- **DMA de raster**: Muestra el bitmap.
- **Chip RAM**: Bitmap en chip.

## Técnicas y trucos

- **Relleno de triángulo**: Ordenar los 3 vértices por Y. Dividir el triángulo en dos “mitades” (arriba y abajo del vértice central). Para cada Y, interpolar x en la arista izquierda y en la derecha; BlitterFill entre (x_left, Y) y (x_right, Y).
- **Orden de pintado**: Dibujar las caras de atrás hacia delante (mayor Z primero) para que las cercanas tapen correctamente.
- **Degradado por copper**: Se puede cambiar LoadColors por línea (por ejemplo COLOR01 más oscuro arriba y más claro abajo) para dar sensación de luz en una cara sin calcular muchos colores en CPU.
- **Prisma triangular**: 5 caras: 2 triángulos (tapas) y 3 rectángulos (laterales). Cada cara es un polígono a rellenar.

## Conceptos para principiantes

- **Proyección 3D** — Pasar de (x,y,z) a (x’,y’) en pantalla. La fórmula típica divide por z para que lo lejano se vea más pequeño.
- **Painter’s algorithm** — Dibujar primero lo que está más lejos y después lo cercano, para que lo cercano tape lo lejano sin necesidad de depth buffer.
- **Relleno por scanlines** — Rellenar un polígono dibujando una línea horizontal por cada valor de Y dentro del polígono, desde x_min hasta x_max en esa línea. El blitter hace el “trazado” de cada línea muy rápido.
