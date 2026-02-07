# Tutorial: Twister RGB (Torbellino RGB)

## Objetivo

Efecto **twister** (torbellino) en **RGB**: una forma que **gira** y se **deforma** como una cinta o torbellino, con colores que pueden variar por **canal RGB** (en AGA) o con **paleta** que simula degradados (en OCS/ECS). Se dibuja con **líneas** o **bandas** que siguen una curva (espiral, seno) y se colorean según la posición o el tiempo. Puede usar **copper** para **degradados por línea** (LoadColors) y **BlitterLine** o **CpuLine** para las curvas.

## Archivos clave

- `effects/twister-rgb/twister-rgb.c` — Init (bitmap, copper, tablas sin/cos), Render (calcula puntos de la espiral/torbellino, dibuja líneas o bandas; actualiza paleta o colores RGB si AGA).
- Posible uso de modo HAM o extra half-brite si hay muchos colores.

## Flujo

1. **Init**: Crea bitmap y copper. Precalcula tabla de senos/cosenos. Define parámetros del twister (radio, número de “vueltas”, amplitud). LoadColors: paleta con degradado (o en AGA, preparar colores RGB por línea). Activa DMA.
2. **Render**: Para cada “segmento” del twister: ángulo = base_angle + segment_index * step + time * speed. Radio y altura (Y) pueden variar con seno. Calcula (x,y) en pantalla. Dibuja **línea** al siguiente segmento con **BlitterLine** o **CpuLine**; color según índice (segment_index o ángulo). Si hay degradado por línea, la copper ya tiene LoadColors por línea; si no, se elige color de la paleta según posición. TaskWaitVBlank.
3. **Kill**: Libera bitmap y copper.

## Mecanismos de hardware que lo hacen posible

- **Copper**: **LoadColors por línea** (o PCHG) para **degradado vertical**: cada línea puede tener un color distinto (o un rango de colores), dando sensación de “cinta” con sombreado. En **AGA** se pueden cargar muchos más colores (256, 262144 en 24 bit); el twister puede tener colores RGB reales que cambian por línea.
- **Blitter**: **BlitterLine** para dibujar cada segmento del torbellino (las líneas que unen los puntos de la espiral). Rápido y en chip RAM.
- **CPU**: Cálculo de la **geometría** del twister (ángulo, radio, y = f(angle), x = cos(angle)*r, etc.) y de los **índices de color** (o valores RGB en AGA). Actualización del **tiempo** (frame) para la animación.
- **DMA de raster**: Muestra el bitmap; los colores vienen de la paleta (o de los datos en AGA). Si la copper cambia LoadColors por línea, el twister se ve con el degradado.
- **Chip RAM**: Bitmap en chip.

## Técnicas y trucos

- **Espiral 3D proyectada**: x = center_x + cos(angle) * radius; y = center_y + sin(angle) * radius + wave * sin(angle * twists). Así la “cinta” sube y baja. Color = (angle >> 3) & 0xF para ciclar por la paleta.
- **Degradado en copper**: Para cada línea Y, LoadColors con COLOR01 = interpolate(color_start, color_end, Y/screen_height). Da sensación de luz o de cinta con volumen.
- **RGB en AGA**: Si el efecto está en AGA, los colores pueden ser 24-bit; se puede variar R,G,B por segmento o por línea para un twister muy colorido.
- **Doble espiral**: Dos “cintas” con ángulos desfasados (angle y angle+PI) para efecto más lleno.

## Conceptos para principiantes

- **Twister (torbellino)** — Forma que gira y a menudo se retuerce como una cinta o espiral. Los puntos siguen (x = r*cos(a), y = r*sin(a)) con r y un “offset” en Y que varía con el ángulo.
- **Degradado por línea** — La copper carga colores distintos en distintas líneas; así la misma imagen tiene un gradiente de color de arriba a abajo (o al revés) sin usar más planos de bit.
- **AGA** — Chipset avanzado del Amiga 1200/4000; más colores (256 simultáneos, 24-bit en algunos modos) y más resolución. Permite efectos “Twister RGB” con muchos más tonos.
