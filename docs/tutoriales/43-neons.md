# Tutorial: Neons

## Objetivo

Efecto **neón**: líneas o formas que brillan con un **halo** o **glow**, como letreros de neón. Se suele lograr dibujando la forma varias veces con **diferente grosor o desenfoque** y sumando/mezclando colores, o usando una **paleta** que da sensación de brillo (degradados hacia blanco) y quizá **color cycling** suave para que “pulse”.

## Archivos clave

- `effects/neons/neons.c` — Init (bitmap, copper, paleta con tonos “neón”), Render (dibuja la forma base y luego capas de “glow” con líneas más gruesas o copias desplazadas en tonos más suaves).
- Posible uso de **BlitterLine** o **CpuLine** para las líneas; **BlitterCopy** con modo minterm para “sumar” capas si se usa más de un plano.

## Flujo

1. **Init**: Crea bitmap y copper. Define paleta: colores neón (por ejemplo magenta, cyan, verde) con varios niveles de intensidad (oscuro → brillante) para el glow. Carga LoadColors. Activa DMA.
2. **Render**: (Opcional) limpia o atenúa el bitmap para estela. Dibuja la forma “exterior” del glow primero (líneas gruesas o varias líneas paralelas en color tenue) en una o varias pasadas. Luego dibuja el **núcleo** de la línea (más fino, color más brillante). Si hay animación, actualiza posiciones o ángulos. Opcional: color cycling en la paleta para que el neón “pulse”. TaskWaitVBlank.
3. **Kill**: Libera bitmap y copper.

## Mecanismos de hardware que lo hacen posible

- **Copper**: **LoadColors** define los tonos neón y los niveles de brillo. Se puede cambiar la paleta por línea (por ejemplo la parte superior de la pantalla con un tono y la inferior con otro) para dar más sensación de luz. **bplpt** para mostrar el bitmap.
- **Blitter / CPU**: Dibujan las líneas: **BlitterLine** para el contorno; para el “glow” se puede hacer varias líneas paralelas (desplazadas 1–2 píxeles) en color más suave, o una pasada con líneas más “gruesas” (dibujar varias líneas juntas). La sensación de brillo viene de la **paleta** (degradado) y de la **superposición** de capas (varios planos o varias pasadas).
- **DMA de raster**: Muestra el bitmap con todas las capas ya mezcladas.
- **Chip RAM**: Bitmap en chip.

## Técnicas y trucos

- **Glow por capas**: Dibujar la misma forma 3–5 veces: primero con color muy tenue y líneas “gordas” (varias líneas desplazadas), luego con color medio y menos grosor, y al final el núcleo fino y brillante. Sin mezcla alpha en hardware, se simula con el orden de dibujo y con usar planos distintos (OR) o una paleta que ya tenga el “halo” en los colores.
- **Paleta neón**: Colores saturados (magenta, cyan, verde, amarillo) con 2–4 niveles desde oscuro hasta casi blanco. El color cycling puede mover el índice de “brillo máximo” para que parezca que pulsa.
- **Formas**: Texto, logos o curvas (Bézier o segmentos de línea). Las líneas se dibujan con BlitterLine o CpuLine; el efecto neón es sobre todo artístico (paleta + múltiples pasadas).

## Conceptos para principiantes

- **Glow (resplandor)** — Efecto de luz que se desvanece alrededor de una forma. En Amiga se simula dibujando la forma varias veces con tamaños y tonos distintos, sin tener “alpha blending” real.
- **Color cycling** — Cambiar los valores de la paleta (por ejemplo cada frame) para que los mismos índices de color muestren un color distinto. Da sensación de animación sin redibujar píxeles.
- **Paleta por línea** — La copper puede ejecutar LoadColors en distintas líneas de la pantalla. Así la misma imagen puede verse con colores diferentes arriba y abajo (útil para neón que “ilumina” más abajo).
