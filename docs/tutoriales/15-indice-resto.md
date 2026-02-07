# Tutoriales resumidos: resto de efectos

Cada entrada indica **objetivo**, **técnica principal** y **archivo(s)** a mirar. Sirve como guía para profundizar por tu cuenta.

---

## Abduction
**Objetivo**: Escena temática de “abducción”.  
**Técnica**: Combinación de gráficos, posiblemente sprites o capas y animación por frames.  
**Archivo**: `effects/abduction/abduction.c`

## Anim
**Objetivo**: Animación con sprites o frames pregenerados.  
**Técnica**: Datos generados con `gen-anim.py`; secuencia de imágenes mostradas por frame.  
**Archivos**: `effects/anim/anim.c`, `effects/anim/data/gen-anim.py`

## Anim-polygons
**Objetivo**: Polígonos 2D animados a partir de SVG.  
**Técnica**: Datos exportados desde SVG (cock.svg, dancing.svg) con `encode.py`; transformaciones 2D y dibujo (blitter o CPU).  
**Archivos**: `effects/anim-polygons/anim-polygons.c`, `data/encode.py`

## Ball
**Objetivo**: Bola que rebota.  
**Técnica**: Posición (x,y) y velocidad; datos de la bola con `gen-ball.py`; blitter para dibujar en cada frame.  
**Archivos**: `effects/ball/ball.c`, `data/gen-ball.py`

## Blurred
**Objetivo**: Efecto de desenfoque.  
**Técnica**: Promediar vecinos (box blur o similar) en CPU o con varias pasadas de blitter; puede usarse doble buffer.  
**Archivo**: `effects/blurred/blurred.c`

## Blurred3D
**Objetivo**: Objeto 3D con blur (motion blur o acumulación).  
**Técnica**: Lib3D para transformar y dibujar; varias pasadas o alpha para suavizar; datos `szescian.mtl`.  
**Archivos**: `effects/blurred3d/blurred3d.c`, `data/szescian.mtl`

## Bobs3D
**Objetivo**: Objetos 3D movibles (bobs) en pantalla.  
**Técnica**: Varios Object3D (o instancias) con distintas posiciones/rotaciones; lib3D; datos `pilka.mtl`.  
**Archivos**: `effects/bobs3d/bobs3d.c`, `data/pilka.mtl`

## BumpMap RGB
**Objetivo**: Bump mapping en RGB (relieve con luz).  
**Técnica**: Mapa de normales o altura; script `bumpmap.py`/`light.py` para datos; cálculo de luz por píxel o por bloque y salida en chunky/RGB.  
**Archivos**: `effects/bumpmap-rgb/bumpmap-rgb.c`, `data/bumpmap.py`, `data/light.py`

## Butterfly-gears
**Objetivo**: Engranajes tipo “mariposa”.  
**Técnica**: Bucle de textura o patrón generado; `textureloop.asm` y `gen-unrolled-loop.py` para optimización (loop desenrollado).  
**Archivos**: `effects/butterfly-gears/butterfly-gears.c`, `textureloop.asm`, `data/gen-unrolled-loop.py`

## Carrion
**Objetivo**: Muestra una imagen partida en trozos.  
**Técnica**: Imagen preprocesada con `split-img.py`; se dibujan las partes en posiciones o con paleta distinta.  
**Archivos**: `effects/carrion/carrion.c`, `data/split-img.py`

## Cathedral
**Objetivo**: Efecto “catedral” (rayos, perspectiva).  
**Técnica**: Ray casting 2D o tabla de ángulos; líneas que convergen o bitmap precalculado; copper para colores por línea.  
**Archivo**: `effects/cathedral/cathedral.c`

## Credits
**Objetivo**: Pantalla de créditos (texto).  
**Técnica**: Texto en bitmap o fuente; scroll o líneas estáticas; LoadColors y copper estándar.  
**Archivo**: `effects/credits/credits.c`

## Darkroom
**Objetivo**: Efecto “cuarto oscuro” (revelado, contraste).  
**Técnica**: Manipulación de paleta (oscurecer/iluminar) o tabla de LUT sobre índices de color; puede haber fade.  
**Archivo**: `effects/darkroom/darkroom.c`

## Dna3D
**Objetivo**: Doble hélice 3D (DNA).  
**Técnica**: Dos (o más) mallas 3D que rotan; datos necrocoq y bobs; copper o paleta para colorear por profundidad.  
**Archivos**: `effects/dna3d/dna3d.c`, `data/dna_gen.py`, `data/necrocoq-*.c`

## FlatShade
**Objetivo**: Objeto 3D con shading plano (una luz por cara).  
**Técnica**: Lib3D (UpdateFaceVisibility da color por cara); rellenar triángulos con blitter o CPU; VBlank para actualizar.  
**Archivos**: `effects/flatshade/flatshade.c`, `data/codi.mtl`

## FlatShade-convex
**Objetivo**: Shading plano para mallas convexas.  
**Técnica**: Sin ordenación de caras (painter); asume que el objeto es convexo; datos `pilka.mtl`.  
**Archivos**: `effects/flatshade-convex/flatshade-convex.c`, `data/pilka.mtl`

## Floor-old
**Objetivo**: Versión anterior del efecto floor.  
**Técnica**: Similar a floor pero con otra estrategia de scroll o paleta; más código o más pasadas.  
**Archivo**: `effects/floor-old/floor-old.c`

## Forest
**Objetivo**: Bosque (árboles, partículas).  
**Técnica**: Muchos elementos 2D/3D (sprites o blits); L-systems o datos precalculados; ordenación por Y o por Z.  
**Archivo**: `effects/forest/forest.c`

## Glitch
**Objetivo**: Glitch visual (corrupción).  
**Técnica**: Escribir valores aleatorios en registros (color, bplcon1) o en zonas del bitmap; a veces solo durante unos frames.  
**Archivo**: `effects/glitch/glitch.c`

## Glitches
**Objetivo**: Varios tipos de glitches.  
**Técnica**: Datos de “tearing” con `gen_tearing.py`; combinación de desplazamientos y colores incorrectos.  
**Archivos**: `effects/glitches/glitches.c`, `data/gen_tearing.py`

## Growing-tree
**Objetivo**: Árbol que crece (ramas).  
**Técnica**: L-system o lista de segmentos que se extienden cada frame; dibujo con líneas (CpuLine o BlitterLine).  
**Archivo**: `effects/growing-tree/growing-tree.c`

## GUI
**Objetivo**: Interfaz gráfica de prueba.  
**Técnica**: Libgui (botones, texto); eventos de ratón/teclado; bitmap y copper normales.  
**Archivo**: `effects/gui/gui.c`

## Kbtest
**Objetivo**: Prueba de teclado.  
**Técnica**: Lectura del teclado (input.device o raw); mostrar código de tecla en pantalla.  
**Archivo**: `effects/kbtest/kbtest.c`

## Layers (Credits)
**Objetivo**: Varias capas (planos) con prioridad.  
**Técnica**: Dos playfields o prioridad de planos; LoadColors por capa; ver también credits.  
**Archivo**: `effects/layers/layers.c`

## Lines
**Objetivo**: Muchas líneas (efecto visual).  
**Técnica**: CpuLine o BlitterLine; posible optimización en asm (CpuEdgeOpt.asm, CpuLineOpt.asm).  
**Archivos**: `effects/lines/lines.c`, `CpuEdgeOpt.asm`, `CpuLineOpt.asm`

## Magnifying-glass
**Objetivo**: Lupa que distorsiona la imagen.  
**Técnica**: Mapa de desplazamiento (lens); datos `ball-anim.csv`, `gen-lens.py`; muestreo del bitmap según coordenadas deformadas.  
**Archivos**: `effects/magnifying-glass/magnifying-glass.c`, `data/gen-lens.py`

## MultiPipe
**Objetivo**: Múltiples “pipes” o túneles.  
**Técnica**: Varias instancias de un efecto tipo pipe; `stripes.c` compartido; copper o bitmap.  
**Archivos**: `effects/multipipe/multipipe.c`, `stripes.c`

## Neons
**Objetivo**: Tubo de neón animado.  
**Técnica**: Rotación de paleta en VBlank (CustomRotatePalette); gráfico estático; datos `neons.py`.  
**Archivos**: `effects/neons/neons.c`, `data/neons.py`

## PlayAHX / PlayCinter / PlayP61 / PlayProtracker
**Objetivo**: Reproducir música (AHX, Cinter, OctaMED/P61, Protracker).  
**Técnica**: Cada uno usa su librería (libahx, libctr, libp61, libpt); Init instala driver e inicia módulo; VBlank o timer para tick de música.  
**Archivos**: `effects/playahx/playahx.c`, `effects/playctr/playctr.c`, `effects/playp61/playp61.c`, `effects/playpt/playpt.c`

## Plotter
**Objetivo**: Efecto tipo plotter (trazos, flares).  
**Técnica**: Líneas o puntos que se van dibujando; datos `plotter-flares.py`; CpuLine o BlitterLine.  
**Archivos**: `effects/plotter/plotter.c`, `data/plotter-flares.py`

## Prisms
**Objetivo**: Prismas (objetos 3D o efecto de refracción).  
**Técnica**: Lib3D o proyección 2D; colores por cara o por línea.  
**Archivo**: `effects/prisms/prisms.c`

## Roller
**Objetivo**: Efecto “rodillo” (scroll vertical o cilindro).  
**Técnica**: Scroll de bitmap o copper cambiando bplpt por línea; texto o patrón que sube/baja.  
**Archivo**: `effects/roller/roller.c`

## Rotator
**Objetivo**: Rotar una imagen o bitmap.  
**Técnica**: Rotación 2D (seno/coseno) por cada píxel o por bloques; `mainloop.asm` para núcleo optimizado; posible precalculación de tablas.  
**Archivos**: `effects/rotator/rotator.c`, `mainloop.asm`

## Sea-anemone
**Objetivo**: Anémona de mar (tentáculos).  
**Técnica**: Partículas o segmentos que ondulan; física simple (seno por “tentáculo”); datos o generación procedural.  
**Archivo**: `effects/sea-anemone/sea-anemone.c`

## ShowPCHG
**Objetivo**: Mostrar animación de paleta PCHG.  
**Técnica**: Formato PCHG (color cycling por línea/frame); solo se cargan colores, sin Render de píxeles.  
**Archivo**: `effects/showpchg/showpchg.c`

## Spooky-tree
**Objetivo**: Árbol “fantasma”.  
**Técnica**: Imagen partida con `split-img.py`; animación por paleta o por posición; VBlank.  
**Archivos**: `effects/spooky-tree/spooky-tree.c`, `data/split-img.py`

## Stencil3D
**Objetivo**: 3D con stencil (máscara).  
**Técnica**: Dibujar objeto 3D y usar un plano o máscara para recortar; datos `cock-anim.csv`, `kurak-head.mtl`.  
**Archivos**: `effects/stencil3d/stencil3d.c`, `data/kurak-head.mtl`

## TexObj (texobj)
**Objetivo**: Triángulos texturizados (objeto 3D con textura).  
**Técnica**: UV mapping; por cada triángulo visible, muestrear textura y rellenar; `mainloop.asm` para el núcleo; datos `cube.mtl`.  
**Archivos**: `effects/texobj/texobj.c`, `mainloop.asm`, `data/cube.mtl`

## Thunders
**Objetivo**: Rayos o relámpagos.  
**Técnica**: Líneas brillantes aleatorias; paleta que parpadea a blanco; CpuLine o BlitterLine; datos o procedural.  
**Archivo**: `effects/thunders/thunders.c`

## Tiles8 / Tiles16
**Objetivo**: Mosaico de tiles 8×8 o 16×16.  
**Técnica**: Tile map (índices); blitter para copiar tiles desde un “tileset” al bitmap de pantalla; twist-colors/twist.py para animación de paleta.  
**Archivos**: `effects/tiles8/tiles8.c`, `effects/tiles16/tiles16.c`, `data/twist.py`

## TileZoomer
**Objetivo**: Zoom sobre tiles.  
**Técnica**: Escalar el tile map (cambiar tamaño de “celda”) o usar BPLCON1/scroll para efecto de zoom; blitter para escalado si hace falta.  
**Archivo**: `effects/tilezoomer/tilezoomer.c`

## Transparency
**Objetivo**: Demostración de transparencia.  
**Técnica**: Blitter con máscara (BlitterCopyMasked) o modo EHB/dual playfield; dos capas donde una deja ver la otra.  
**Archivo**: `effects/transparency/transparency.c`

## Turmite
**Objetivo**: Turmite (autómata sobre rejilla).  
**Técnica**: Rejilla de estados; reglas del turmite (estado, color, giro); actualización en CPU o con blitter; datos RLE para patrones.  
**Archivo**: `effects/turmite/turmite.c`

## Twister RGB
**Objetivo**: Efecto “twister” en RGB.  
**Técnica**: Distorsión de imagen o buffer chunky; rotación/remap de píxeles; posible c2p.  
**Archivo**: `effects/twister-rgb/twister-rgb.c`

## UVLight / UVMap / UVMapRGB
**Objetivo**: Malla 3D con coordenadas UV e iluminación o textura.  
**Técnica**: Lib3D + coordenadas UV por vértice; luz direccional (UVLight) o textura (UVMap); datos con `gen-uvmap.py`; relleno de triángulos con color o texel.  
**Archivos**: `effects/uvlight/uvlight.c`, `effects/uvmap/uvmap.c`, `effects/uvmap-rgb/uvmap-rgb.c`, `data/gen-uvmap.py`

## Weave
**Objetivo**: Patrón tipo tejido.  
**Técnica**: Patrón repetido (tabla o fórmula); escritura en bitmap o copper por línea; XOR o máscaras para “entrelazar”.  
**Archivo**: `effects/weave/weave.c`

---

Para **cualquier efecto**, un buen orden es: (1) abrir el `.c`, (2) buscar `EFFECT(` para ver qué callbacks usa, (3) leer `Init` para recursos y copper, (4) leer `Render` para el bucle principal, (5) revisar `data/` y scripts `.py` para el origen de los datos.
