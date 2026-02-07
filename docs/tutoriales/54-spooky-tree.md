# Tutorial: Spooky-tree (Árbol fantasma)

## Objetivo

Efecto de **árbol “fantasma”**: un árbol (o estructura ramificada) que se dibuja con **líneas** o **segmentos**, a menudo con animación (ramas que se mueven, crecen o parpadean). Puede usar **recursión** o **L-system** para generar la estructura de ramas y **CpuLine** o **BlitterLine** para dibujarlas. El aspecto “fantasma” puede venir de **color tenue**, **estela** (no borrar todo el bitmap) o **doble imagen** (EOR).

## Archivos clave

- `effects/spooky-tree/spooky-tree.c` — Init (bitmap, copper), Render (genera o actualiza posiciones de ramas, dibuja líneas; opcional estela o EOR).
- Posible recursión o L-system para la forma del árbol.

## Flujo

1. **Init**: Crea bitmap y copper. Inicializa parámetros del árbol (ángulo inicial, longitud del tronco, factor de reducción por nivel, número de niveles). Si usa L-system: define reglas y estado. LoadColors (paleta suave o “fantasma”). Activa DMA.
2. **Render**: (Opcional) limpia solo parcialmente o no limpia (estela). **Genera las ramas**: recursivamente o con L-system calcula los puntos (x,y) de cada segmento. Para cada segmento: **CpuLine** o **BlitterLine** desde (x1,y1) a (x2,y2). Si hay animación, el ángulo o la longitud pueden depender de sin(frame) o del nivel. TaskWaitVBlank.
3. **Kill**: Libera bitmap y copper.

## Mecanismos de hardware que lo hacen posible

- **Blitter** (BlitterLine): Dibuja cada rama como una línea. Muy eficiente cuando hay muchas ramas. BlitterLineSetup una vez; luego BlitterLine por cada segmento.
- **CPU**: **Generación de la estructura** (recursión: “dibuja tronco, gira izquierda, dibuja rama más corta, gira derecha, …” o L-system expandiendo una cadena de símbolos). Cálculo de coordenadas (cos/sin para ángulos). Si se usa CpuLine en lugar de blitter, la CPU también dibuja.
- **Copper**: bplpt y LoadColors. Colores tenues (grises, verdes oscuros) dan el aspecto “fantasma”. Se puede cambiar color por línea para degradado.
- **DMA de raster**: Muestra el bitmap.
- **Chip RAM**: Bitmap en chip.

## Técnicas y trucos

- **Árbol recursivo**: función branch(len): si len < mínimo, return; dibuja línea hacia delante (len); gira -angle; branch(len * factor); gira +2*angle; branch(len * factor); gira -angle; return. Así se obtiene un árbol binario simétrico.
- **L-system**: Cadena de símboles (F=avanzar, + = girar +, - = girar -). Regla F -> F+F-F-F+F; empezar con F. Interpretar la cadena dibujando; cada F es un segmento. Variando las reglas se obtienen formas de árbol o fractal.
- **Efecto fantasma**: No borrar el bitmap cada frame; las líneas se acumulan y se desvanecen (o se dibuja con EOR y se vuelve a dibujar en la siguiente posición para “borrar” la anterior). O usar color muy bajo (índice 1 o 2) que sea casi negro.

## Conceptos para principiantes

- **L-system (Lindenmayer system)** — Sistema de reescritura: se parte de un axioma (ej. “F”) y se aplican reglas (F -> F+F-F) para generar una cadena larga. Esa cadena se interpreta como instrucciones de dibujo (F=línea, +=girar). Da formas fractales y de árbol.
- **Recursión** — Una función que se llama a sí misma (con parámetros distintos). Para el árbol: “dibuja rama; a la izquierda dibuja un árbol más pequeño; a la derecha otro”. Hay que tener una condición de parada (longitud mínima o nivel máximo).
- **Estela** — Dejar el frame anterior en pantalla (o atenuado) y dibujar encima. Las líneas viejas se ven “fantasma” detrás de las nuevas.
