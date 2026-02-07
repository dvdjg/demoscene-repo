# Tutorial: Carrion

## Objetivo

Mostrar **una imagen partida en trozos** (por ejemplo una cara o un logo cortado en tiras o bloques). Los trozos pueden mostrarse en orden, con desplazamiento o con paletas distintas. El “partido” de la imagen se hace **antes** en el PC con un script (split-img.py).

## Archivos clave

- `effects/carrion/carrion.c` — Init (carga los trozos o el bitmap ya partido), Render (muestra los trozos en sus posiciones; puede ser estático o con animación leve).
- `data/split-img.py` — Toma una imagen y la corta en regiones; exporta cada región a un .c (bitmap o planos) o a un único bitmap con las partes una debajo de otra.

## Flujo

1. **Init**: Carga los datos generados por split-img.py: pueden ser varios BitmapT (uno por trozo) o un solo bitmap donde cada “trozo” es un área (Area2D). Configura playfield, paleta y copper. Si la imagen es estática, puede copiarse una vez con BitmapCopyArea para cada trozo en su posición.
2. **Render**: Si es estático, no hace falta redibujar (Init ya dejó la pantalla lista). Si hay animación (por ejemplo mostrar trozos uno a uno), se actualiza qué trozos se copian o sus posiciones. TaskWaitVBlank.
3. **Kill**: Libera bitmaps y copper.

## Mecanismos de hardware que lo hacen posible

- **Blitter**: **BitmapCopyArea** copia cada “trozo” desde el bitmap fuente a la posición correcta en el bitmap de pantalla. Sin el blitter habría que copiar palabra a palabra en CPU; el blitter hace copias rectangulares muy rápidas.
- **Copper**: Mantiene los punteros de bitplanes (bplpt) y la paleta. La imagen final es el bitmap que hemos recompuesto con el blitter.
- **DMA de raster**: Lee ese bitmap desde chip RAM y lo envía a la Denise para mostrarlo.
- **Chip RAM**: Los bitmaps de los trozos y el de pantalla están en chip para que el blitter pueda leer y escribir.
- **CPU**: Solo indica origen, destino y tamaño de cada copia; el trabajo pesado es del blitter.

## Técnicas y trucos

- **Precortar en el PC**: split-img.py hace el trabajo de “cortar” la imagen en partes. El Amiga solo recibe arrays de píxeles o planos ya definidos en .c. Así no se gasta tiempo ni memoria en parsear imágenes en el Amiga.
- **Un bitmap con todas las partes**: Si todos los trozos están en un solo bitmap (uno debajo de otro), cada BitmapCopyArea usa el mismo src pero con Area2D distinto (y destino distinto). Muy cómodo para el blitter.
- **Efecto “revelado”**: Se puede ir mostrando trozo a trozo según frameCount (cada N frames se añade un trozo más) para un efecto de “construcción” de la imagen.

## Conceptos para principiantes

- **BitmapCopyArea(dst, dx, dy, src, area)** — Copia el rectángulo definido por `area` (x, y, ancho, alto) desde `src` a la posición (dx, dy) en `dst`. Ideal para “pegado” de trozos.
- **Area2D** — Estructura { x, y, w, h } que define un rectángulo. Se usa para decir “esta región del bitmap” sin tener que crear un bitmap nuevo por cada trozo.
- **Generación offline** — Muchos efectos de demo usan scripts (Python, etc.) en el PC para generar datos (imágenes cortadas, tablas, código). El ejecutable del Amiga solo usa esos datos ya listos.
