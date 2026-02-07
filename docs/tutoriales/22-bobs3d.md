# Tutorial: Bobs3D

## Objetivo

Varios **objetos 3D** (“bobs”) que se mueven por la pantalla: mismos datos de malla (por ejemplo una esfera) pero distintas posiciones y rotaciones. Cada bob es una instancia que se transforma y se dibuja (wireframe o relleno).

## Archivos clave

- `effects/bobs3d/bobs3d.c` — Array de objetos o de posiciones/ángulos; en Render se recorre y se dibuja cada uno.
- `data/pilka.mtl` — Malla (pilka = pelota en polaco).
- Librería lib3d: NewObject3D, UpdateObjectTransformation, UpdateFaceVisibility, SortFaces (o variante).

## Flujo

1. **Init**: Crea varios Object3D a partir de la misma malla (pilka), o un solo objeto que se “reutiliza” cambiando translate/rotate. Crea bitmap y copper. Posiciones y ángulos iniciales para cada bob.
2. **Render**: Para cada bob: actualiza su posición/rotación (por ejemplo en función de frameCount y un offset por bob). **UpdateObjectTransformation**, **UpdateFaceVisibility**, proyecta vértices a 2D. Dibuja (líneas con BlitterLine o caras con relleno). Se puede dibujar todos en el mismo bitmap y luego hacer un solo SortFaces si se trata como un solo “objeto” con varias partes, o dibujar uno tras otro (ordenados por Z medio). Actualiza bplpt, TaskWaitVBlank.
3. **Kill**: Libera todos los objetos 3D, bitmap y copper.

## Mecanismos de hardware que lo hacen posible

- **CPU (68000)**: Para **cada** bob calcula la matriz de transformación (objectToWorld), la visibilidad de caras y la proyección 3D→2D. Varios bobs = varias veces el mismo cálculo con distintos parámetros. La lib3d está optimizada para hacerlo en fixed-point sin floats.
- **Blitter**: Dibuja las **líneas** (BlitterLine) o las **caras rellenas** de cada bob. Sin el blitter, dibujar muchas líneas o muchos triángulos en CPU sería demasiado lento.
- **Copper**: Mantiene los punteros de bitplanes y la paleta. Si cada bob tiene un color distinto, la paleta tiene que tener esos colores; el “cambio” de color por bob se hace al elegir qué color de línea o relleno usar al dibujar.
- **DMA de raster**: Muestra el bitmap donde se han dibujado todos los bobs.
- **Chip RAM**: La malla (compartida) y el bitmap están en chip.

## Técnicas y trucos

- **Una malla, muchas instancias**: No se duplica la geometría; solo se guardan N conjuntos de (translate, rotate) o N Object3D que apuntan a los mismos datos de vértices/caras. Se dibuja N veces con distintas matrices.
- **Ordenación entre bobs**: Si los bobs se solapan, hay que dibujarlos de atrás adelante por su Z (por ejemplo Z medio del objeto). Se puede ordenar la lista de bobs por Z antes de dibujar.
- **Movimiento independiente**: Cada bob puede tener su propia velocidad y fase (frameCount + offset) para que no se muevan todos igual.

## Conceptos para principiantes

- **Instancia** — Misma forma (malla) usada varias veces con distinta posición/rotación. En 3D es la forma estándar de tener muchos “enemigos” o objetos iguales sin repetir la geometría.
- **Object3D** — Estructura que tiene los datos de la malla (compartidos) y la transformación propia (rotate, scale, translate) y las matrices calculadas (objectToWorld). Varios Object3D pueden compartir el mismo Mesh3D.
- **Painter’s algorithm** — Pintar primero lo que está más lejos y después lo que está más cerca; lo que se pinta después tapa lo anterior. Para que funcione hay que ordenar por Z.
