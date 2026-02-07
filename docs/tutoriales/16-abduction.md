# Tutorial: Abduction

## Objetivo

Escena de “abducción”: fondo, OVNI, anillo y gallina (coq) que sube por un haz de luz. La animación sigue fases (ABDUCT, RETRACT_BEAM, ESCAPE, END) y combina varios gráficos copiados con el blitter en posiciones que cambian cada frame.

## Archivos clave

- `effects/abduction/abduction.c` — Fases, DrawBackground, DrawUfo, DrawRing, haz de luz, paleta del haz.
- `data/bkg.c`, `data/ufo.c`, `data/ring.c`, `data/coq.c`, `data/mid_beam.c`, `data/side_beam_*.c` — Bitmaps y datos del fondo, OVNI, anillo, gallina y haces.

## Flujo

1. **Init**: Crea dos bitmaps de pantalla (doble buffer), lista copper con bitplanes y **sprites** para los haces (o capas). Carga paletas; posición inicial del OVNI y de la gallina.
2. **Render**: Según la fase (phase) y un contador (counter): dibuja fondo con **BitmapCopyArea**, luego OVNI, anillo y gallina en posiciones (ufo_pos, coq_pos) que se actualizan. El haz de luz puede ser sprites o zonas del bitmap; la paleta del haz (beam_pal_cp) se actualiza desde el copper. Al final se intercambia el buffer activo y se actualizan los punteros de bitplanes en la copper.
3. **Kill**: Libera bitmaps, listas y recursos.

## Mecanismos de hardware que lo hacen posible

- **Blitter**: Copia rectángulos de un bitmap a otro (**BitmapCopyArea**). Sin el blitter, mover cada gráfico (fondo, OVNI, anillo, gallina) sería muy lento en CPU. El blitter hace esas copias en memoria de chips mientras la CPU puede preparar el siguiente frame.
- **Copper**: Mantiene la lista de instrucciones que dice a la Denise **qué bitplanes mostrar** (bplpt) y **qué colores usar** (incluida la paleta del haz por línea o por zona). También puede controlar **sprites** (punteros SPRxPT) si los haces son sprites.
- **DMA de raster**: La Denise lee por DMA los planos de bits cada línea; sin activar DMAF_RASTER no se vería la imagen.
- **Sprites (opcional)**: Si los haces de luz son sprites, el hardware de sprites (Agnus/Denise) los dibuja encima del playfield sin tocar el bitmap, con su propia paleta (COLOR17–31).
- **Chip RAM**: Todos los bitmaps (bkg, ufo, ring, coq, screen) deben estar en chip memory porque el blitter y el raster DMA solo acceden a esa zona.

## Técnicas y trucos

- **Doble buffer**: Se dibuja en un bitmap mientras se muestra el otro; se evita que se vea el borrado o el dibujo a medias.
- **Fases de animación**: Un enum (phaseE) y un contador deciden qué mover y cuándo pasar a la siguiente escena (bajar OVNI, subir gallina, retraer haz, etc.).
- **BitmapCopyArea con recorte**: Al dibujar el anillo o la gallina parcialmente fuera de pantalla, se pasa un Area2D que recorta el origen para no leer fuera del gráfico.
- **Paleta dinámica del haz**: Varias paletas (beam_pal[4][7]) que se escriben en la copper (beam_pal_cp) para dar efecto de haz encendido/apagado o pulsación.

## Conceptos para principiantes

- **BitmapCopyArea(dst, dx, dy, src, area)** — Copia solo el rectángulo definido por `area` desde `src` a la posición (dx, dy) de `dst`. El blitter hace la copia plano a plano.
- **Chip RAM** — Memoria que comparten la CPU, el blitter y el raster DMA. Los gráficos que se ven en pantalla y los que copia el blitter tienen que estar aquí.
- **Sprite** — Objeto que el hardware dibuja aparte del bitmap (hasta 8); tiene posición (VSTART, HSTART, VSTOP) y datos de imagen. Muy útil para overlays (haces, proyectiles).
