# Tutorial: Forest

## Objetivo

Efecto de **bosque**: muchos elementos (árboles, partículas o sprites) que dan sensación de profundidad. Los elementos pueden ser gráficos 2D dibujados con el blitter en posiciones calculadas (perspectiva 2D: más lejos = más arriba y más pequeño) y opcionalmente ordenados por Y para que los lejanos se dibujen primero.

## Archivos clave

- `effects/forest/forest.c` — Init (muchos “árboles” o partículas con posición x, y, scale), Render (ordenar por Y, dibujar cada uno con BitmapCopy o BlitterCopyMasked).
- Datos: uno o varios gráficos de árbol/silueta y paleta.

## Flujo

1. **Init**: Crea array de elementos (posición x, y, escala o “profundidad” z). Puede inicializar posiciones aleatorias o en rejilla. Crea bitmap(s), carga los sprites/gráficos de árbol. Configura playfield y copper.
2. **Render**: Ordena los elementos por Y (o por z) para dibujar de atrás adelante. Para cada elemento: calcula posición en pantalla (x puede tener offset por “profundidad”; escala puede reducir el tamaño del gráfico). **BitmapCopy** o **BlitterCopyMasked** del gráfico al bitmap de pantalla en esa posición. Actualiza bplpt, TaskWaitVBlank. Opcionalmente mueve ligeramente las posiciones (viento, scroll).
3. **Kill**: Libera bitmaps, gráficos y copper.

## Mecanismos de hardware que lo hacen posible

- **Blitter**: **BitmapCopy** o **BitmapCopyArea** (y si hay transparencia, **BlitterCopyMasked**) copia cada “árbol” en su posición. Sin el blitter, dibujar decenas de objetos por frame en CPU sería inviable.
- **CPU (68000)**: Calcula las posiciones (y opcionalmente ordenación por Y o Z), aplica “perspectiva” 2D (y más grande, z más pequeño) y dice al blitter origen/destino/tamaño para cada copia.
- **Copper**: Mantiene bplpt y paleta. Si los árboles usan la misma paleta, un solo LoadColors basta.
- **DMA de raster**: Muestra el bitmap con todos los árboles ya dibujados.
- **Chip RAM**: Gráficos de árboles y bitmap de pantalla en chip.

## Técnicas y trucos

- **Perspectiva 2D**: No es 3D real; cada elemento tiene (x, y, z) donde z es “profundidad”. Posición en pantalla: screen_x = centerX + (x * scale(z)), screen_y = y, y el tamaño del gráfico puede ser scale(z). Así los de z grande se ven más pequeños y más “arriba”.
- **Ordenar por Y**: Dibujar primero los que tienen Y mayor (más abajo en pantalla = más cerca) para que no queden tapados por los de atrás. O ordenar por z y dibujar primero los de z grande (lejos).
- **Pocos gráficos, muchas instancias**: Un solo bitmap de “árbol” se copia muchas veces en distintas posiciones; ahorra memoria y tiempo de carga.

## Conceptos para principiantes

- **Sprite / bob** — En demos, “bob” suele referirse a un gráfico que se copia en distintas posiciones (con el blitter). No tiene por qué ser un “sprite” hardware del Amiga; puede ser solo un BitmapCopy.
- **Orden de dibujado** — Lo que se dibuja después tapa lo anterior. Por eso se dibuja primero lo que está “lejos” y después lo que está “cerca”.
- **BlitterCopyMasked** — Copia un gráfico que tiene “huecos” (máscara): donde la máscara es 0 no se escribe, así el fondo se ve. Necesario para árboles no rectangulares.
