# Tutorial: BumpMap RGB

## Objetivo

**Bump mapping** en color (RGB): una superficie que parece tener relieve gracias a un “mapa de normales” o de altura. La luz se calcula según la orientación de cada “pixel” y se pinta con color (no solo intensidad). Los datos (mapa, luz) se precalculan con scripts Python.

## Archivos clave

- `effects/bumpmap-rgb/bumpmap-rgb.c` — Init (carga mapa y paleta), Render (por cada píxel o por bloques: cálculo de luz, índice de color, escritura en buffer chunky o planar).
- `data/bumpmap.py` — Genera el mapa de normales o de altura.
- `data/light.py` — Calcula dirección de luz y/o tablas de intensidad.

## Flujo

1. **Load**: Genera o carga el mapa de bump (altura o normales) y las tablas de luz (ángulo → color o intensidad). Puede ser un bitmap precalculado o un array.
2. **Init**: Crea buffer de destino (chunky o planar) y copper. Si el resultado es chunky, puede hacer falta conversión c2p con blitter para mostrarlo.
3. **Render**: Para cada píxel (o para bloques): lee el valor del bump map, calcula la “normal” o el factor de luz, busca el color en una tabla o con una fórmula, escribe en el buffer. Si el buffer es chunky, luego se convierte a planar (blitter) y se actualiza la copper. TaskWaitVBlank.
4. **Kill / UnLoad**: Libera buffers y datos.

## Mecanismos de hardware que lo hacen posible

- **CPU (68000)**: Hace el **cálculo de luz** por píxel (o por bloque): producto escalar normal·luz, búsqueda en tabla, escritura del índice de color. Es el núcleo del efecto; puede estar optimizado con tablas precalculadas para no hacer multiplicaciones en tiempo real.
- **Blitter**: Si el resultado se escribe en un buffer chunky, el **chunky-to-planar (c2p)** lo hace el blitter en varias pasadas. También puede copiar o combinar zonas del mapa.
- **Copper**: Mantiene bplpt y paleta. La paleta debe tener los colores del “degradado” de luz (desde sombra hasta brillo).
- **DMA de raster**: Muestra el bitmap final. Si se usa modo HAM o muchos colores, el copper puede cargar una paleta grande.
- **Chip RAM**: Mapas y buffers en chip para que el blitter y el video los usen.

## Técnicas y trucos

- **Precalcular todo lo posible**: En el script (bumpmap.py, light.py) se generan el mapa y tablas (ángulo → color). En el Amiga solo se hace: leer mapa, indexar tabla, escribir píxel.
- **Tabla de luz**: En lugar de calcular (normal·luz) en tiempo real, se tiene un array: tabla[índice_del_mapa] = color. El índice puede ser la “altura” o un código de orientación.
- **Resolución reducida**: Calcular bump a 160×128 y luego escalar o duplicar píxeles para pantalla completa ahorra mucho tiempo de CPU.

## Conceptos para principiantes

- **Bump map** — Imagen o array donde cada posición guarda “cuánto sobresale” la superficie (altura) o la orientación (normal). Con eso y una dirección de luz se calcula la sombra o el brillo.
- **Normal** — Vector perpendicular a la superficie en un punto. El producto escalar normal·luz da cuánto “recibe” ese punto de luz: mayor = más brillante.
- **Chunky** — Formato donde cada píxel es un byte (o word) seguido en memoria. Fácil de escribir desde C; luego hay que convertir a planar para que el Amiga lo muestre (c2p).
