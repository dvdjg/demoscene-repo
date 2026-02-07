# Tutorial: Growing-tree

## Objetivo

Un **árbol que crece**: ramas que van apareciendo y alargándose frame a frame. Suele modelarse con un **L-system** (reglas de sustitución que generan una secuencia de “avanza, gira, dibuja”) o con una lista de segmentos a los que cada frame se añade longitud o una nueva rama. Se dibuja con **líneas** (CpuLine o BlitterLine).

## Archivos clave

- `effects/growing-tree/growing-tree.c` — Init (lista de segmentos o estado del L-system, bitmap, copper), Render (aplica reglas o extiende segmentos, dibuja las líneas actuales).
- No suele tener datos externos; la geometría se genera en tiempo de ejecución o con una semilla aleatoria.

## Flujo

1. **Init**: Inicializa la estructura del árbol (por ejemplo: tronco = un segmento; lista de ramas vacía o con reglas L-system). Crea bitmap y copper. Color de fondo y color de las líneas (paleta).
2. **Render**: **Crecimiento**: según frameCount, añade un nuevo segmento (o aplica un paso del L-system: interpretar el string “F” = avanza y dibuja, “+” = gira derecha, “-” = gira izquierda, etc.) y guarda el nuevo segmento (x1,y1,x2,y2). **Dibujo**: limpia el bitmap (o solo la zona del árbol) y dibuja **todas** las líneas actuales con CpuLine o BlitterLine. Actualiza bplpt, TaskWaitVBlank.
3. **Kill**: Libera lista de segmentos (si está en RAM), bitmap y copper.

## Mecanismos de hardware que lo hacen posible

- **CPU (68000)**: Ejecuta la **lógica de crecimiento** (L-system o “añadir segmento”) y mantiene la lista de segmentos. Calcula las coordenadas (x1,y1,x2,y2) de cada línea. Es trabajo de enteros (ángulos, longitudes).
- **Blitter** (si se usa BlitterLine): **BlitterLine** dibuja cada segmento. Más rápido que CpuLine para muchas líneas. BlitterLineSetup una vez, luego BlitterLine por cada par de puntos.
- **CpuLine** (alternativa): Si se usa la CPU para dibujar, CpuLineSetup y luego CpuLine(x1,y1,x2,y2) por cada segmento. Aceptable si el número de segmentos no es muy alto.
- **Copper**: Mantiene bplpt y paleta. La “imagen” es el bitmap donde se han dibujado las líneas.
- **DMA de raster**: Muestra ese bitmap. Cada frame hay más líneas (o líneas más largas), por eso el árbol “crece”.

## Técnicas y trucos

- **L-system**: Cadena de símbolos (F, +, -, [, ]); “F” = avanza y dibuja, “+” = gira ángulo fijo, “-” = gira al revés, “[” = guarda posición, “]” = restaura. Cada paso se interpreta y se dibuja. Para “crecer” se va generando más cadena (aplicando reglas: F → F+F-F) o se va mostrando más carácter a carácter.
- **Lista de segmentos**: En lugar de L-system se puede tener un array de (x1,y1,x2,y2). Cada frame se añade un segmento nuevo (hijo de uno existente, con ángulo aleatorio o fijo). Se redibuja todo cada frame (o solo el nuevo segmento si se quiere optimizar).
- **Redibujar todo**: La forma más simple es borrar (o no) y dibujar todas las líneas cada frame. Así no hay que “borrar” solo el último segmento; el árbol es acumulativo.

## Conceptos para principiantes

- **L-system (Lindenmayer system)** — Conjunto de reglas que reemplazan símbolos por cadenas (por ejemplo F → F+F-F). Al aplicar las reglas varias veces se obtiene una cadena larga que, interpretada como “avanza/gira/dibuja”, genera formas de plantas o fractales.
- **Segmento** — Una línea entre dos puntos (x1,y1) y (x2,y2). Un árbol es un conjunto de segmentos (tronco + ramas). Crecer = añadir segmentos nuevos que salen de los extremos de los existentes.
- **BlitterLine** — El blitter del Amiga puede dibujar líneas (Bresenham en hardware). Se configura una vez (bitmap, plano, modo OR/EOR) y luego se llama con los extremos de cada línea.
