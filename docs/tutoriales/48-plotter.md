# Tutorial: Plotter

## Objetivo

Efecto tipo **plotter**: dibujar una figura o curva como si la dibujara un **plotter** (lápiz que se mueve de punto a punto). Suele mostrarse una **línea que se va trazando** en tiempo real (segmentos que se añaden frame a frame) o un **dibujo estático** generado por secuencia de movimientos. Se usa **CpuLine** o **BlitterLine** para cada segmento.

## Archivos clave

- `effects/plotter/plotter.c` — Init (bitmap, copper), Render (avanza la “posición del lápiz”, dibuja un segmento desde la posición anterior hasta la nueva con CpuLine/BlitterLine).
- Posible `data/` con lista de puntos (x,y) o instrucciones (movimiento, bajar subir “lápiz”).

## Flujo

1. **Init**: Crea bitmap y copper. Carga o genera la secuencia de puntos a dibujar (array de (x,y) o de vectores). Inicializa “posición del lápiz” (por ejemplo primer punto). LoadColors. Activa DMA.
2. **Render**: Toma el siguiente punto (o los siguientes N) de la secuencia. Dibuja **línea** desde la posición actual del lápiz hasta el nuevo punto con **CpuLine** o **BlitterLine**. Actualiza la posición del lápiz al nuevo punto. Si se quiere efecto de “dibujo continuo”, se avanza de a poco (1–3 segmentos por frame). TaskWaitVBlank.
3. **Kill**: Libera bitmap y copper.

## Mecanismos de hardware que lo hacen posible

- **Blitter** (si se usa BlitterLine): **Modo línea** del blitter dibuja el segmento entre dos puntos con Bresenham en hardware. Muy rápido; la CPU solo da los extremos. BlitterLineSetup una vez; luego BlitterLine por cada segmento.
- **CPU** (si se usa CpuLine): Algoritmo de línea en software (Bresenham), escribe cada píxel en el bitmap. Adecuado cuando hay pocos segmentos o segmentos muy cortos.
- **Copper**: bplpt y paleta. La imagen es el bitmap con las líneas ya dibujadas.
- **DMA de raster**: Muestra el bitmap.
- **Chip RAM**: Bitmap en chip para blitter y para la pantalla.

## Técnicas y trucos

- **Datos del “dibujo”**: Pueden ser coordenadas (x,y) de un logo, una curva (Bézier discretizada), o un “turtle” (ángulo + distancia). Convertir a secuencia de puntos y dibujar segmentos entre consecutivos.
- **Velocidad**: Dibujar 1, 2 o N segmentos por frame para controlar la velocidad del “plotter”. Más segmentos por frame = dibujo más rápido.
- **Efecto “lápiz arriba/abajo”**: Si los datos tienen “move” vs “draw”, en los “move” no se dibuja línea, solo se actualiza la posición; en “draw” se dibuja el segmento. Así se pueden hacer letras o figuras con huecos.
- **Borrado**: Para repetir el efecto, limpiar el bitmap al terminar la secuencia y reiniciar el índice de puntos.

## Conceptos para principiantes

- **Plotter** — Máquina que dibuja moviendo un “lápiz” en dos ejes (X,Y). En este efecto se simula dibujando segmentos de línea entre puntos consecutivos.
- **Bresenham (en blitter)** — El blitter tiene circuito de líneas que implementa este algoritmo: dado (x1,y1) y (x2,y2), activa los píxeles de la línea sin usar multiplicaciones ni divisiones.
- **Turtle graphics** — Idea de “tortuga” que tiene posición y ángulo: “avanza 10, gira 90” etc. Se puede convertir a (x,y) con: x += cos(angle)*dist, y += sin(angle)*dist.
