# Tutorial: Butterfly-gears

## Objetivo

**Engranajes** con forma de “mariposa” que giran. El patrón se genera con un bucle de textura o con datos precalculados; el bucle crítico puede estar en **ensamblador** (textureloop.asm) para ganar velocidad, y un script (gen-unrolled-loop.py) genera una versión “desenrollada” del bucle.

## Archivos clave

- `effects/butterfly-gears/butterfly-gears.c` — Init (bitmap, copper, tal vez buffer de textura), Render (llama al bucle de dibujo o actualiza punteros).
- `effects/butterfly-gears/textureloop.asm` — Bucle en 68000 que escribe la textura o el patrón en el bitmap (muy optimizado).
- `data/gen-unrolled-loop.py` — Genera código o datos para un bucle desenrollado (más rápido, más tamaño).
- `data/textureloop-generated.txt` — Salida del script (código o coordenadas).

## Flujo

1. **Init**: Reserva bitmap(s) y posiblemente un buffer de textura. Carga los datos generados por gen-unrolled-loop (o la textura precalculada). Configura playfield y copper. El “dibujo” puede ser: rellenar un buffer chunky con el patrón (llamando al asm) y luego c2p con blitter.
2. **Render**: Actualiza ángulo o fase. Llama a la rutina en **asm** (OptimizedTextureLoop o similar) que escribe el patrón de los engranajes en el buffer usando seno/coseno o tablas. Si hace falta, el blitter convierte chunky→planar. Actualiza bplpt, TaskWaitVBlank.
3. **Kill**: Libera buffers y copper.

## Mecanismos de hardware que lo hacen posible

- **CPU (68000)**: El **bucle de generación** del patrón (ángulo → coordenadas, coordenadas → índice de color) corre en la CPU. En C sería lento; por eso se pasa a **ensamblador**: menos instrucciones, uso de registros y direccionamiento óptimo. El asm puede leer tablas (sintab) y escribir en memoria con MOVE.B o MOVE.W en bucle muy ajustado.
- **Blitter**: Si el resultado está en formato chunky, el **c2p** lo hace el blitter. También puede copiar una “plantilla” del engranaje varias veces en distintas posiciones/ángulos (con rotación precalculada en tablas).
- **Copper**: Mantiene bplpt y paleta. La paleta define los colores del engranaje (metálico, sombras).
- **DMA de raster**: Muestra el bitmap. El “movimiento” viene de que cada frame se regenera el patrón con un ángulo distinto.
- **Chip RAM**: El buffer donde escribe el asm debe estar en chip para que luego el blitter o el raster lo use.

## Técnicas y trucos

- **Bucle desenrollado**: En lugar de “for (i=0; i<N; i++)” el script genera N copias del cuerpo del bucle seguidas. Así se evitan saltos y el CPU pipeline trabaja mejor; el código es más grande pero más rápido.
- **Tablas de seno/coseno**: La forma del engranaje depende del ángulo; en lugar de calcular sin/cos en tiempo real se usan tablas (sintab) indexadas por el ángulo. En asm: getword(sintab, angle).
- **Asm para el núcleo**: Lo que más tarda (escribir muchos píxeles) se hace en .asm; el resto (Init, Kill, actualizar ángulo) puede quedarse en C.

## Conceptos para principiantes

- **Bucle desenrollado (unrolled loop)** — En vez de dar N vueltas a un bucle, se escribe N veces el cuerpo del bucle una tras otra. Menos instrucciones de control (decrementar, saltar) y a veces más rápido.
- **Rutina en asm** — Se declara en C como `extern void TextureLoop(...);` y se define en un .asm o .s. El enlazador la une al programa. Se usa para las partes que deben ser lo más rápidas posible.
- **Chip RAM** — La memoria donde escribe el asm tiene que ser la que el hardware de video puede leer; en Amiga eso es la “chip memory”.
