# Tutoriales por efecto

Cada tutorial explica **cómo se consigue** un efecto y qué **trucos y técnicas** se usan, pensado para un programador principiante en demos/Amiga. Incluyen la sección **Mecanismos de hardware que lo hacen posible** (Copper, Blitter, DMA, CPU, Paula, Chip RAM) para que quede claro qué hace el hardware en cada caso.

## Índice por número

| # | Efecto | Técnicas / hardware principal |
|---|--------|-------------------------------|
| 01 | [Empty](01-empty.md) | Plantilla efecto, Init/Kill/Render, custom registers |
| 02 | [Circles](02-circles.md) | Círculos por CPU (Bresenham), 1 bitplane, SetColor |
| 03 | [Color-cycling (Logo)](03-color-cycling.md) | Animación por rotación de paleta, sin redibujar píxeles |
| 04 | [Plasma](04-plasma.md) | Tablas sin/cos, copper “chunky” por tile 8×4, doble buffer |
| 05 | [Fire RGB](05-fire-rgb.md) | Buffer de fuego, chunky→planar con blitter, HAM, línea cuadruplicada |
| 06 | [Wireframe](06-wireframe.md) | 3D: transformación, back-face culling, líneas con blitter |
| 07 | [Shapes](07-shapes.md) | Lib2D: matrices, ClipPolygon2D, BlitterLine + BlitterFill |
| 08 | [Floor](08-floor.md) | Bitmap estático + paleta dinámica, BPLCON1 por línea, “scroll” |
| 09 | [TextScroll](09-textscroll.md) | Fuente 8×8, copper por línea (bplpt), scroll vertical |
| 10 | [Loader](10-loader.md) | CpuLine, Protracker, barra de progreso |
| 11 | [Game of Life](11-game-of-life.md) | Blitter con minterms para vecinos, historial en bitplanes |
| 12 | [Stripes](12-stripes.md) | Copper por línea (color), proyección 2D de segmentos |
| 13 | [Highway](13-highway.md) | Múltiples zonas en copper, sprites, bitplanes por franja |
| 14 | [Metaballs](14-metaballs.md) | Blobs con blitter, doble buffer, máscaras |
| 15 | [Índice resto (16–67)](15-indice-resto.md) | Enlaces a todos los tutoriales del resto de efectos |
| 16 | [Abduction](16-abduction.md) | Gráficos, sprites/capas, animación por frames |
| 17 | [Anim](17-anim.md) | Frames pregenerados, secuencia por frame, blitter |
| 18 | [Anim-polygons](18-anim-polygons.md) | SVG→datos, transformaciones 2D, blitter/CPU |
| 19 | [Ball](19-ball.md) | Posición/velocidad, blitter por frame |
| 20 | [Blurred](20-blurred.md) | Promedio de vecinos, blitter o CPU, doble buffer |
| 21 | [Blurred3D](21-blurred3d.md) | Lib3D, motion blur o acumulación |
| 22 | [Bobs3D](22-bobs3d.md) | Varios Object3D, lib3D, datos .mtl |
| 23 | [BumpMap RGB](23-bumpmap-rgb.md) | Mapa de normales, luz por píxel/bloque, chunky/RGB |
| 24 | [Butterfly-gears](24-butterfly-gears.md) | Bucle de textura, loop desenrollado (asm) |
| 25 | [Carrion](25-carrion.md) | Imagen partida, blitter por trozos |
| 26 | [Cathedral](26-cathedral.md) | Ray casting 2D, copper por línea |
| 27 | [Credits](27-credits.md) | Texto, scroll, LoadColors |
| 28 | [Darkroom](28-darkroom.md) | Paleta/LUT, fade, contraste |
| 29 | [Dna3D](29-dna3d.md) | Doble hélice 3D, lib3D, copper/paleta por profundidad |
| 30 | [FlatShade](30-flatshade.md) | Lib3D, una luz por cara, relleno triángulos |
| 31 | [FlatShade-convex](31-flatshade-convex.md) | Shading plano, malla convexa sin ordenación |
| 32 | [Floor-old](32-floor-old.md) | Versión anterior floor, scroll/paleta |
| 33 | [Forest](33-forest.md) | Muchos elementos 2D/3D, L-systems, ordenación Y/Z |
| 34 | [Glitch](34-glitch.md) | Valores aleatorios en registros/bitmap |
| 35 | [Glitches](35-glitches.md) | Tearing, desplazamientos, paleta incorrecta |
| 36 | [Growing-tree](36-growing-tree.md) | L-system o segmentos, CpuLine/BlitterLine |
| 37 | [GUI](37-gui.md) | Libgui, ratón/teclado, bitmap |
| 38 | [Kbtest](38-kbtest.md) | Lectura teclado, input.device |
| 39 | [Layers](39-layers.md) | Dual playfield, prioridad BPLCON2 |
| 40 | [Lines](40-lines.md) | CpuLine/BlitterLine, asm optimizado |
| 41 | [Magnifying-glass](41-magnifying-glass.md) | Zoom/copia con blitter, máscara circular |
| 42 | [MultiPipe](42-multipipe.md) | Tubos 3D, proyección, BlitterFill por banda |
| 43 | [Neons](43-neons.md) | Líneas + glow por capas, paleta, color cycling |
| 44 | [PlayAHX](44-playahx.md) | Driver AHX, Paula (AUDx*), chip RAM samples |
| 45 | [PlayCinter](45-playcinter.md) | Driver Cinter, Paula, módulo en chip |
| 46 | [PlayP61](46-playp61.md) | Driver P61, Paula, 4 canales DMA |
| 47 | [PlayProtracker](47-playprotracker.md) | Driver Protracker/MOD, Paula |
| 48 | [Plotter](48-plotter.md) | Secuencia de puntos, CpuLine/BlitterLine |
| 49 | [Prisms](49-prisms.md) | Triángulos 3D, proyección, BlitterFill |
| 50 | [Roller](50-roller.md) | bplpt por línea (copper), efecto cilindro |
| 51 | [Rotator](51-rotator.md) | Rotación 2D, copper bplpt o blitter |
| 52 | [Sea-anemone](52-sea-anemone.md) | Rayos radiales, seno/coseno, BlitterLine |
| 53 | [ShowPCHG](53-showpchg.md) | Palette change por línea, copper enlazada |
| 54 | [Spooky-tree](54-spooky-tree.md) | Árbol recursivo/L-system, líneas, estela |
| 55 | [Stencil3D](55-stencil3d.md) | Máscara 3D, blitter minterms |
| 56 | [TexObj](56-texobj.md) | UV mapping, interpolación, blitter/CPU |
| 57 | [Thunders](57-thunders.md) | Líneas zigzag, flash por LoadColors o BlitterFill |
| 58 | [Tiles8](58-tiles8.md) | Mapa de tiles, BlitterCopy 8×8 |
| 59 | [Tiles16](59-tiles16.md) | Mapa de tiles, BlitterCopy 16×16 |
| 60 | [TileZoomer](60-tilezoomer.md) | Zoom sobre tiles, blitter repetición/salto |
| 61 | [Transparency](61-transparency.md) | Dual playfield, color 0 transparente |
| 62 | [Turmite](62-turmite.md) | Autómata en rejilla, CPU, buffer en chip |
| 63 | [Twister RGB](63-twister-rgb.md) | Espiral/torbellino, copper degradado, AGA |
| 64 | [UVLight](64-uvlight.md) | Paleta “UV”, color cycling |
| 65 | [UVMap](65-uvmap.md) | Mapeo UV, interpolación, textura en chip |
| 66 | [UVMapRGB](66-uvmaprgb.md) | UV + HAM/AGA, muchos colores |
| 67 | [Weave](67-weave.md) | Bandas que se cruzan, blitter, orden de pintado |

## Cómo usar estos tutoriales

1. **Empieza por Empty** para ver la estructura mínima de un efecto.
2. **Circles y Color-cycling** muestran gráficos simples y animación sin blitter.
3. **Plasma y Fire** introducen copper avanzado y chunky-to-planar.
4. **Wireframe y Shapes** enseñan 2D/3D con las librerías del repo.
5. **Floor, TextScroll, Highway** profundizan en copper por línea y varias zonas.
6. **Game of Life** es un buen ejemplo de uso “creativo” del blitter.
7. Del **16 al 67** tienes un tutorial completo por efecto, con **mecanismos de hardware** explicados.

En cada tutorial encontrarás:
- **Objetivo**: qué se ve en pantalla.
- **Archivos clave**: qué .c/.h y data/ mirar.
- **Flujo**: Init → Render (y VBlank/Load si aplica) → Kill.
- **Mecanismos de hardware que lo hacen posible**: Copper, Blitter, DMA, CPU, Paula, Chip RAM.
- **Técnicas y trucos**: ideas reutilizables.
- **Conceptos para principiantes**: términos de Amiga explicados brevemente.
