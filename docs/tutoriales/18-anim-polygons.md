# Tutorial: Anim-polygons

## Objetivo

Mostrar **polígonos 2D** animados cuyas formas vienen de archivos **SVG** (por ejemplo un gallo o figuras bailando). Los vértices se transforman con matrices 2D y se dibujan (contorno y relleno) con el blitter.

## Archivos clave

- `effects/anim-polygons/anim-polygons.c` — Carga de datos, transformación 2D, dibujo de polígonos.
- `effects/anim-polygons/data/cock.svg`, `dancing.svg` — Formas en SVG.
- `effects/anim-polygons/data/encode.py` — Convierte SVG a datos C (puntos y polígonos) que el efecto usa.

## Flujo

1. **Init**: Los datos (puntos originales y lista de polígonos) ya vienen en el .c generado por encode.py. Se crea bitmap, se configura ClipWin, playfield y copper. Opcionalmente se cargan varios “frames” si el SVG tiene animación o varias figuras.
2. **Render**: Se construye la matriz 2D (rotación, escala, traslación) según frameCount. Se aplica **Transform2D** a todos los puntos. Se recorta con **ClipPolygon2D** y se dibuja cada polígono con **BlitterLine** y **BlitterFill** (igual que en Shapes). Se actualiza la copper y se espera VBlank.
3. **Kill**: Libera bitmap y copper.

## Mecanismos de hardware que lo hacen posible

- **CPU (68000)**: Calcula la **transformación 2D** (multiplicar puntos por la matriz) y el **recorte de polígonos**. Son muchas operaciones con enteros (fixed-point); el 68000 hace los productos y sumas.
- **Blitter**: **BlitterLine** dibuja las aristas del polígono y **BlitterFill** rellena el interior. Hacerlo en CPU sería mucho más lento; el blitter tiene circuito dedicado para líneas y relleno.
- **Copper**: Mantiene los punteros de bitplanes (bplpt) y la paleta. Sin copper no habría imagen en pantalla.
- **DMA de raster**: Lee los planos de bits desde chip RAM y los envía a la Denise para formar la imagen.
- **Chip RAM**: El bitmap de destino y los datos de los polígonos (si están en RAM) deben estar en chip para que el blitter escriba ahí.

## Técnicas y trucos

- **SVG → C**: encode.py lee el SVG (paths, polígonos), discretiza curvas en segmentos y genera arrays de Point2D y listas de índices. Así el Amiga no interpreta SVG; solo usa números ya listos.
- **Lib2D**: Se usa LoadIdentity2D, Rotate2D, Scale2D, Translate2D, Transform2D, PointsInsideBox y ClipPolygon2D. Es el mismo pipeline que en el efecto Shapes.
- **Varios “frames” desde SVG**: Si el SVG tiene varias capas o estados (por ejemplo un baile), el script puede exportar varios conjuntos de puntos; el efecto elige cuál mostrar según frameCount.

## Conceptos para principiantes

- **SVG (Scalable Vector Graphics)** — Formato de gráficos vectoriales (líneas, curvas, rellenos). Para usarlo en Amiga hay que “convertirlo” a listas de puntos y polígonos que nuestra librería 2D entienda.
- **Transform2D** — Aplica la matriz actual (rotación, escala, traslación) a un array de puntos. Los puntos “en pantalla” (viewPoint) son el resultado; los originales (origPoint) no cambian.
- **ClipPolygon2D** — Recorta el polígono contra la ventana ClipWin para que no se dibuje fuera de pantalla. El algoritmo (Sutherland-Hodgman) corta las aristas que salen del rectángulo visible.
