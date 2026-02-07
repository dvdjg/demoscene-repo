# Tutoriales por efecto

Cada tutorial explica **cómo se consigue** un efecto y qué **trucos y técnicas** se usan, pensado para un programador principiante en demos/Amiga.

## Índice por nombre

| # | Efecto | Técnicas principales |
|---|--------|----------------------|
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
| 15+ | [Resto de efectos](15-indice-resto.md) | Enlaces a tutoriales resumidos |

## Cómo usar estos tutoriales

1. **Empieza por Empty** para ver la estructura mínima de un efecto.
2. **Circles y Color-cycling** muestran gráficos simples y animación sin blitter.
3. **Plasma y Fire** introducen copper avanzado y chunky-to-planar.
4. **Wireframe y Shapes** enseñan 2D/3D con las librerías del repo.
5. **Floor, TextScroll, Highway** profundizan en copper por línea y varias zonas.
6. **Game of Life** es un buen ejemplo de uso “creativo” del blitter.

En cada tutorial encontrarás:
- **Objetivo**: qué se ve en pantalla.
- **Archivos clave**: qué .c/.h mirar.
- **Flujo**: Init → Render (y VBlank si aplica) → Kill.
- **Técnicas y trucos**: ideas reutilizables.
- **Conceptos para principiantes**: términos de Amiga explicados brevemente.
