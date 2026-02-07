# Tutorial: Blurred3D

## Objetivo

Un **objeto 3D** (por ejemplo un cubo o malla) que se ve **desenfocado** o con “motion blur”. Se puede lograr dibujando el objeto varias veces en posiciones ligeramente distintas y sumando (o promediando) en el bitmap, o dibujando con transparencia/atenuación.

## Archivos clave

- `effects/blurred3d/blurred3d.c` — Init (Object3D, bitmap, copper), Render (transformación, visibilidad, dibujo múltiple o con acumulación).
- `data/szescian.mtl` — Malla 3D (por ejemplo cubo; “szescian” = cubo en polaco).

## Flujo

1. **Init**: Crea Object3D desde la malla (szescian). Crea bitmap(s) y copper. Configura playfield y paleta. Igual que en wireframe o flatshade.
2. **Render**: Actualiza rotación/posición. **UpdateObjectTransformation**, **UpdateFaceVisibility**, **SortFaces**. Para el blur: en lugar de dibujar una sola vez, se dibuja el objeto varias veces con un pequeño cambio de ángulo o posición, o se dibuja una vez con un “spread” (por ejemplo rellenar no solo el píxel central sino vecinos con menor intensidad). Las pasadas se pueden combinar con el blitter (OR, o promedio con minterms). Actualiza bplpt, TaskWaitVBlank.
3. **Kill**: Libera objeto 3D, bitmaps y copper.

## Mecanismos de hardware que lo hacen posible

- **CPU (68000)**: Calcula las **transformaciones 3D** (matrices, proyección), **visibilidad de caras** (producto escalar con la cámara) y **ordenación por Z**. La librería lib3d hace todo esto en fixed-point.
- **Blitter**: Dibuja las caras (relleno de triángulos o líneas). Para el blur, el blitter puede **sumar** o **mezclar** varias pasadas (por ejemplo con operaciones OR o con minterms que simulan promedio) escribiendo en el mismo bitmap.
- **Copper**: Mantiene bplpt y paleta. Si el “blur” se hace cambiando paleta por línea (degradado de intensidad), el copper es quien carga esos colores.
- **DMA de raster**: Muestra el bitmap donde se ha dibujado el objeto (y sus copias o la versión difuminada).
- **Chip RAM**: La malla, el bitmap y los buffers deben estar en chip para el blitter y el video.

## Técnicas y trucos

- **Múltiples dibujados**: Dibujar el mismo objeto 2–4 veces con un pequeño desplazamiento de ángulo (frameCount + 0, +1, +2…) y hacer OR de los planos da sensación de estela o blur de movimiento.
- **Paleta con “colores suaves”**: Si cada nivel de “intensidad” del blur es un índice de color (más transparente = color más oscuro), no hace falta alpha blending; basta con elegir el color según la “cantidad” de blur en ese píxel.
- **Objeto simple**: Un cubo (pocas caras) permite varias pasadas por frame sin quedarse sin tiempo de CPU/blitter.

## Conceptos para principiantes

- **Motion blur** — En el mundo real, un objeto en movimiento se ve algo borroso porque el ojo (o la cámara) integra la luz en el tiempo. En gráficos se simula dibujando el objeto varias veces en posiciones cercanas o mezclando frames.
- **SortFaces** — Ordena las caras del objeto por profundidad (Z) para pintarlas de atrás hacia adelante (painter’s algorithm) y que no se vean triángulos por detrás de otros.
- **Minterms del blitter** — Permiten combinar tres fuentes (A, B, C) con una función lógica. Con varias pasadas se puede aproximar un “promedio” (por ejemplo D = A or B para acumular siluetas).
