# Tutorial: FlatShade

## Objetivo

Un **objeto 3D** dibujado con **shading plano** (flat shading): cada cara (triángulo) tiene un solo color según cuánta “luz” recibe (producto escalar entre la normal de la cara y la dirección de la luz). Las caras se dibujan de atrás adelante (painter’s algorithm) y se rellenan con el blitter o con una rutina de relleno de triángulo.

## Archivos clave

- `effects/flatshade/flatshade.c` — Init (Object3D, bitmap, copper), Render (UpdateObjectTransformation, UpdateFaceVisibility, SortFaces, luego rellenar cada cara con su color).
- `data/codi.mtl` — Malla 3D (objeto).
- **VBlank**: Puede actualizar algo (por ejemplo posición de luz o punteros) en la interrupción de vertical blank.

## Flujo

1. **Init**: Crea Object3D desde la malla (codi). Crea bitmap y copper. Configura playfield y paleta (los 16 colores pueden ser 16 niveles de “luz” desde oscuro a brillante). Igual que wireframe pero con relleno.
2. **Render**: Actualiza rotación del objeto. **UpdateObjectTransformation** (calcula matrices y cámara). **UpdateFaceVisibility** (para cada cara calcula normal·luz y guarda en face->flags un índice de color 0–15). **SortFaces** (ordena caras por Z para pintar de atrás adelante). Para cada cara visible: obtiene los 3 vértices proyectados (x,y en pantalla) y **rellena el triángulo** con el color face->flags (con el blitter o con CpuEdge + BlitterFill). Actualiza bplpt, TaskWaitVBlank.
3. **VBlank** (opcional): Actualiza punteros o estado para el siguiente frame.
4. **Kill**: Libera objeto, bitmap y copper.

## Mecanismos de hardware que lo hacen posible

- **CPU (68000)**: **UpdateFaceVisibility** hace el producto escalar normal·luz para cada cara y convierte el resultado en un índice de color (0–15) usando la tabla InvSqrt. **SortFaces** ordena las caras por Z medio. **UpdateObjectTransformation** calcula las matrices. Todo en fixed-point (lib3d).
- **Blitter**: **Relleno de triángulos**: se dibujan los 3 bordes del triángulo (BlitterLine) y luego **BlitterFill** rellena el interior. O se usa una rutina que rellena línea a línea. El blitter es mucho más rápido que rellenar en CPU.
- **Copper**: Mantiene bplpt y la **paleta**. La paleta debe tener los 16 colores como una escala de “oscuro a brillante” (o de color con distinta intensidad) para que el índice 0–15 de cada cara se vea como luz.
- **DMA de raster**: Muestra el bitmap con los triángulos ya rellenos.
- **Chip RAM**: Malla, bitmap y lista copper en chip.

## Técnicas y trucos

- **InvSqrt**: En UpdateFaceVisibility se usa una tabla precalculada (1/sqrt) para convertir el producto escalar en un factor de luz sin hacer raíz cuadrada en tiempo real.
- **Painter’s algorithm**: SortFaces ordena por Z; se dibuja primero la cara más lejana y después la más cercana. Así las cercanas tapan a las lejanas sin necesidad de un buffer Z.
- **Un color por cara**: “Flat” significa que toda la cara tiene el mismo color. El “shading” es solo elegir qué color (0–15) según la orientación de la cara respecto a la luz.
- **VBlank**: Si se hace algo cada línea (por ejemplo cambiar luz), puede hacerse en VBlank para no restar tiempo al Render.

## Conceptos para principiantes

- **Flat shading** — Cada polígono (cara) se pinta de un solo color. No hay degradado dentro de la cara; la “luz” se ve por el cambio de color entre caras.
- **Normal de una cara** — Vector perpendicular a la cara. Si la luz viene en dirección L, el producto escalar normal·L da cuánto “recibe” la cara: mayor = más brillante.
- **BlitterFill** — Después de dibujar el contorno de un polígono (las 3 líneas del triángulo), el blitter puede rellenar el interior automáticamente. Es el mismo truco que en Shapes.
