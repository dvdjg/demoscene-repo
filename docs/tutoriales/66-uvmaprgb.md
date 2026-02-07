# Tutorial: UVMapRGB (Mapeo UV en RGB)

## Objetivo

Igual que **UVMap** pero con énfasis en **color RGB** o en uso de **más colores** (por ejemplo modo **HAM** o **AGA**): la textura se mapea sobre el objeto 3D y se muestran muchos colores (gradientes RGB, imagen en color real en HAM/AGA). En OCS/ECS, **HAM** (Hold And Modify) permite hasta 4096 colores en pantalla con 6 planos; la “textura” puede ser una imagen HAM y el mapeo UV la muestra sobre el objeto. En **AGA** se puede usar paleta de 256 colores o más. El pipeline es el mismo que UVMap (interpolación UV, sampleo), cambiando solo la **representación del color** (índice de paleta vs HAM vs RGB).

## Archivos clave

- `effects/uvmaprgb/uvmaprgb.c` — Init (textura en formato que permita muchos colores: HAM o paleta grande, geometría con UV, bitmap, copper), Render (mapeo UV igual que UVMap; el resultado se escribe en el bitmap en el formato elegido: 6 planos para HAM, 8 para AGA, etc.).
- Textura: imagen HAM o tabla de colores RGB.

## Flujo

1. **Init**: Carga **textura** (imagen con muchos colores: HAM o 256 colores AGA). Configura **modo de pantalla** (HAM si OCS/ECS para 4096 colores, o AGA 256 colores). Define objeto 3D con (u,v) por vértice. Crea bitmap (6 planos HAM o 8 AGA) y copper. LoadColors (en HAM solo 16 registros base; en AGA 256). Activa DMA.
2. **Render**: **Proyección y orden de caras**. Para cada cara: **interpolación UV** por scanline; para cada píxel (x,y) se obtiene (u,v). **Muestreo**: leer el color de la textura en (u,v). En HAM ese “color” es un código (índice o instrucción HAM); en AGA es índice 0–255. **Escribir** ese valor en el bitmap (varios planos: 6 para HAM). El blitter puede ayudar a escribir palabras completas si la textura está en el mismo formato. TaskWaitVBlank.
3. **Kill**: Libera textura, bitmap y copper.

## Mecanismos de hardware que lo hacen posible

- **HAM (Hold And Modify)** — Modo de 6 planos donde no todos los bits son “índice de paleta”: parte de los bits indican “modifica solo R”, “solo G”, “solo B” del color anterior. Así con 16 colores base se pueden mostrar 4096 colores en pantalla. La “textura” puede ser una imagen HAM; al mapearla sobre el objeto, cada píxel tiene un valor HAM que el hardware interpreta. La CPU o el blitter escriben los 6 planos; el raster en modo HAM muestra muchos colores.
- **AGA** — Hasta 256 colores en paleta (8 planos) o más en modos especiales. La textura puede ser una imagen de 256 colores; el mapeo UV escribe el índice en el bitmap y LoadColors define los 256 colores (incluso 24-bit). No hay “modificación” como en HAM; es paleta grande.
- **Blitter**: Puede **copiar** franjas de la textura (ya en formato pantalla) al bitmap de destino si la textura está en el mismo layout (planos, modulo). Para HAM, escribir 6 planos por píxel es más coste; a veces se hace por línea con el blitter copiando palabras.
- **CPU**: Interpolación UV, **conversión** de (u,v) a dirección de textura y lectura del valor (índice o código HAM). Escritura en el bitmap (varios planos con máscaras y desplazamientos).
- **Copper**: bplpt para 6 u 8 planos, LoadColors (16 para HAM, 256 para AGA). Modo BPLCON0 (HAM o número de planos).
- **DMA de raster**: Muestra el bitmap en modo HAM o AGA; el resultado es el objeto con la textura “a todo color”.
- **Chip RAM**: Textura y bitmap en chip.

## Técnicas y trucos

- **Texto en HAM**: Escribir en HAM requiere actualizar hasta 6 planos por píxel según el código (nuevo color o modify R/G/B). Hay tablas precalculadas: “para mostrar RGB (r,g,b), el código HAM y la palabra de los 6 planos es X”. Así se evita calcular en tiempo real.
- **Textura preconvertida**: La textura puede estar ya en “formato pantalla” (valores listos para escribir en los planos). El mapeo UV solo copia ese valor; no hay conversión RGB->HAM en tiempo real.
- **AGA 256 colores**: Más simple que HAM: textura = índices 0–255; LoadColors tiene 256 entradas. El mapeo escribe el índice; no hay modificación. Mejor calidad de color que HAM para imágenes fotorrealistas.

## Conceptos para principiantes

- **HAM** — Modo de 6 planos que permite 4096 colores en pantalla usando “hold and modify”: cada píxel puede ser un color nuevo (de 16 base) o “modifica solo R/G/B” del píxel anterior. Muy usado en OCS/ECS para muchas tonalidades.
- **Mapeo UV en color** — El mismo algoritmo que UVMap; la diferencia es que la textura y el framebuffer tienen muchos colores (HAM o paleta grande). El objeto 3D se ve “pintado” con la imagen a todo color.
- **AGA** — Chipset que permite 256 colores simultáneos (y más en modos especiales), facilitando texturas muy coloridas sin la complejidad de HAM.
