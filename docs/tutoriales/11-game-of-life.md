# Tutorial: Game of Life

## Objetivo

**Juego de la Vida de Conway**: rejilla de celdas que viven o mueren según el número de vecinos. El “siguiente estado” se calcula **con el blitter** usando minterms y varias pasadas, sin contar vecinos en CPU. El resultado se muestra ampliado (x2 en X y en Y) y los estados anteriores se mantienen en bitplanes adicionales con colores más oscuros (historial).

## Archivos clave

- `effects/game-of-life/game-of-life.c` — Constantes (BOARD, EXT_BOARD, BOARD_COUNT), Init (bitmaps para juego y display), Render (blits para vecinos, duplicación X, copper para duplicación Y, historial en planos).
- Comentarios en el .c describen el tamaño del tablero, el área extendida para bordes y la secuencia de blits.

## Flujo

1. **Init**: crea bitmaps para el “board” (estado actual + buffers intermedios para las pasadas del blitter) y para la pantalla (más planos para el historial). Configura playfield, paleta (colores que se oscurecen por “edad”), copper (incluyendo line doubling para duplicar en Y). Carga patrón inicial (RLE o datos).
2. **Render**:  
   - **Cálculo del siguiente estado**: el estado actual es un bitmap 1 bit (vivo=1, muerto=0). Con varios blits usando el **function generator** del blitter (minterms) y fuentes A/B/C = desplazamientos del board (vecinos), se calcula “número de vecinos” y luego “siguiente estado” según las reglas (2–3 vecinos viven, 3 nacen). La secuencia exacta está en el código (varias pasadas con bltcon0/bltcon1 distintos).  
   - **Duplicación horizontal**: cada celda se duplica en X (una pasada por CPU con tabla o con blitter) para que 1 píxel del board = 2 píxeles en pantalla.  
   - **Duplicación vertical**: en la copper, bpl1mod/bpl2mod (o similar) para que cada línea lógica se muestre dos veces (line doubling).  
   - **Historial**: los estados anteriores (ya duplicados) se mantienen en planos extra; sus colores son más oscuros. Cada frame el “más viejo” se descarta y el actual pasa a ser el más reciente.
3. **Kill**: libera bitmaps y copper.

## Técnicas y trucos

- **Blitter como “ALU”**: el blitter puede combinar tres fuentes (A, B, C) con una función lógica (256 minterms) y escribir en D. Sumar “vecinos” (bits de celdas adyacentes) se puede hacer con varios blits: por ejemplo, copiar el board desplazado 1 palabra a la izquierda, otra a la derecha, etc., y combinar con OR o con minterms que implementan “suma” bit a bit. El código elige minterms y orden de blits para obtener exactamente las reglas de Conway.
- **Minterms**: bltcon0 incluye 8 bits (ABC, ABNC, ANBC, …) que seleccionan qué combinación de A,B,C se escribe en D. Con SRCA, SRCB, SRCC, DEST y los desplazamientos (ASHIFT, BSHIFT) se construyen operaciones como “D = (A and B) or C” o sumas parciales.
- **Área extendida (EXT_BOARD)**: el board lógico tiene un borde extra de celdas para que los blits de “vecinos” no lean fuera de memoria; el área visible (BOARD) es la interior.
- **Historial visual**: guardar las últimas N generaciones en planos adicionales y mostrar cada una con un color más oscuro da sensación de “estela” y ayuda a ver la evolución.

## Conceptos para principiantes

- **Function generator (minterms)** — El blitter no solo copia; puede calcular cualquier función de 3 bits (A,B,C) con una tabla de 256 entradas (los 8 minterms). Así se implementan conteos y reglas sin CPU.
- **Line doubling** — Repetir la misma línea de datos en dos (o más) líneas de pantalla poniendo modulo negativo en la copper para que la “siguiente” línea vuelva a la misma dirección.
- **RLE (Run-Length Encoded)** — Formato de patrón inicial; se decodifica una vez y se escribe en el board. Ver data/*.rle y rle2png.py / electrons2c.py.
