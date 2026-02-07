# Tutorial: Dna3D

## Objetivo

Una **doble hélice 3D** (como el ADN) que gira: dos “cadenas” de geometría (esferas o tubos) que rotan alrededor de un eje. Usa la librería 3D y datos de mallas (bobs, necrocoq) y puede combinar **varias paletas o zonas en la copper** para colorear por profundidad.

## Archivos clave

- `effects/dna3d/dna3d.c` — Varios Object3D o una malla especial de “hélice”; datos necrocoq (mallas) y paletas por “frame” de animación.
- `data/dna_gen.py` — Genera la geometría de la hélice.
- `data/necrocoq-*.c` — Mallas y columnas de color (necrochicken_cols) para animación de color por línea.

## Flujo

1. **Init**: Crea los objetos 3D que forman las dos hebras (o una malla única con muchos “eslabones”). Configura bitmap y **copper con muchas instrucciones por línea**: para cada línea Y puede cargar una paleta distinta (desde necrocoq_00_cols_pixels, etc.) según el “frame” de la animación de color. Activa DMA y copper.
2. **Render**: Actualiza el ángulo de rotación de la hélice (frameCount). **UpdateObjectTransformation**, **UpdateFaceVisibility**, proyecta y dibuja (wireframe o relleno). La copper ya tiene, por línea, los colores que tocan (animación de color precalculada). Se actualiza qué “frame” de color usar (envelope) y se puede modificar la lista copper con esos colores. TaskWaitVBlank.
3. **Kill**: Libera objetos 3D, bitmaps y copper.

## Mecanismos de hardware que lo hacen posible

- **CPU (68000)**: Calcula la **transformación 3D** de la doble hélice (rotación alrededor del eje Y o Z), **visibilidad de caras** y **proyección** a 2D. La lib3d hace el trabajo en fixed-point.
- **Blitter**: Dibuja las **líneas** (BlitterLine) o las **caras rellenas** de la hélice. Sin el blitter, dibujar cientos de segmentos en CPU sería lento.
- **Copper**: Aquí es **clave** para el look: puede cargar **paletas distintas por línea** (o cada pocas líneas) usando los datos necrocoq_xx_cols_pixels. Así la misma geometría se ve con un gradiente o una animación de color que recorre la pantalla, dando sensación de “recorrido” por la hélice o de profundidad.
- **DMA de raster**: Muestra el bitmap donde está dibujada la hélice. Los colores que ve el usuario en cada línea los decide el copper.
- **Chip RAM**: Mallas, bitmap y datos de paleta por línea en chip.

## Técnicas y trucos

- **Dos hebras**: Pueden ser dos Object3D con la misma malla (p. ej. esfera) pero con translate distintos (uno a la izquierda del eje, otro a la derecha) y rotación sincronizada o desfasada.
- **Color por línea desde datos**: Los arrays necrocoq_00_cols_pixels, etc., contienen los valores RGB para cada línea (o cada grupo). La copper los carga con CopMove16 en el WAIT correspondiente. El “envelope” (array de frames) decide qué conjunto de colores usar según frameCount.
- **Geometría generada por script**: dna_gen.py puede generar las posiciones de los “eslabones” de la hélice (puntos en espiral); luego se convierten a malla 3D o a líneas para el Amiga.

## Conceptos para principiantes

- **Doble hélice** — Dos cadenas que se enroscan alrededor de un eje (como el ADN). En 3D se modelan como dos secuencias de puntos (o esferas) con ángulo que avanza y radio constante.
- **Paleta por línea** — En Amiga el copper puede ejecutar MOVE para COLORxx en cualquier línea. Si cada línea tiene su propio set de colores (desde un array), la pantalla puede tener un gradiente o una animación vertical sin cambiar el bitmap.
- **Envelope** — En audio/gráficos, una “envolvente” es una curva que controla un valor en el tiempo (por ejemplo “cuánto” de algo). Aquí puede controlar qué frame de paleta usar (0…10) a lo largo del tiempo.
