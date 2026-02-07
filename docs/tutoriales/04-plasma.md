# Tutorial: Plasma

## Objetivo

Efecto **plasma** clásico: ondas de color que se mueven y se combinan. No hay bitmap de píxeles en el sentido tradicional; la “imagen” se genera **por copper**: cada “tile” de 8×4 píxeles tiene un color que se escribe en un registro vía copper, y ese color se calcula con tablas de seno/coseno.

## Archivos clave

- `effects/plasma/plasma.c` — Load (tablas), Init (copper), Render (UpdateChunky), Kill.
- `data/plasma-colors.c` — Paleta precalculada (array de colores RGB).
- Comentarios en el .c sobre **copper chunky**, DIWVP/DIWHP y restricciones del WAIT/SKIP.

## Flujo

1. **Load**: genera tablas `tab1[], tab2[], tab3[]` con fórmulas tipo `SIN(rad*2)*const` / `COS(rad*2)*const` (fixed-point). Son tablas de 256 bytes para indexar rápido.
2. **Init**: construye **dos** copper lists (doble buffer). Cada lista tiene, por cada “fila” de tiles (cada 4 líneas): un WAIT a esa Y, un MOVE de cop2lc (para hacer que el copper se reinicie en esa línea — truco de “copper chunky”), y luego un MOVE por cada tile de la fila escribiendo el **color de fondo** en un registro. Así la pantalla se divide en celdas 8×4; el color de cada celda se actualizará en Render.
3. **Render**: `UpdateXBUF()` y `UpdateYBUF()` rellenan `xbuf[]` e `ybuf[]` con `tab1[a0]+tab2[a1]+...` (índices que cambian cada frame). Luego, para cada fila y cada columna de tile, calcula `v = xbuf[x] + ybuf[y]` y escribe en la instrucción copper correspondiente el color `colors.pixels[v]`. Así cada celda muestra un color de la paleta según la función “plasma”. Se hace `CopListRun(cp[active])`, se espera VBlank y se intercambia `active` (doble buffer).
4. **Kill**: desactiva DMA de copper y borra las dos listas.

## Técnicas y trucos

- **Plasma sin bitmap**: no hay blitter ni escritura en planos; la “imagen” es solo copper escribiendo colores. La resolución efectiva es en “tiles” (8×4), no por píxel.
- **Tablas precalculadas**: en lugar de calcular sin/cos en tiempo real, se usan tablas indexadas por byte (a0, a1, a2, a3, a4) que se incrementan cada frame; la suma de varias tablas da la función típica de plasma.
- **Copper “chunky”**: el código usa un truco (cop2lc, WAIT por cada grupo de 4 líneas) para que el copper repita el mismo flujo de “escribir color por tile” varias veces por frame. Las restricciones (Y divisible por 4, cuidado con línea 127→128 y 255→256) vienen del HRM: el copper no puede enmascarar el bit más significativo del contador vertical.
- **Doble buffer de listas**: una lista se muestra mientras la otra se rellena; en VBlank se cambia cuál está activa para evitar tearing.
- **Optimización asm en UpdateChunky**: lectura de `xbuf[x]` e `ybuf[y]`, suma, y escritura del color en la instrucción copper con direccionamiento indexado; todo en pocas instrucciones 68000.

## Conceptos para principiantes

- **Copper list** — Secuencia de instrucciones WAIT (esperar posición del haz) y MOVE (escribir valor en registro). El copper las ejecuta en sincronía con el barrido; así se puede cambiar color (o bitplanes, sprites, etc.) por línea o en momentos precisos.
- **CopSetColor / CopMove16** — Inserción en la lista de un MOVE que escribe en un registro de color. En este efecto, esas instrucciones se **modifican** cada frame (se cambia el valor .data) en lugar de tener una lista nueva.
- **Fixed-point (fx4i, SIN, >>16)** — Cálculos sin float: enteros con punto fijo (4 bits fraccionarios) y tabla de seno; el resultado se reduce a byte para indexar la paleta.
