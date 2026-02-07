# Tutorial: Blurred

## Objetivo

Efecto de **desenfoque** (blur): una forma (por ejemplo un círculo o una silueta) aparece borrosa. Se consigue dibujando la forma con **BlitterFill** y **CircleEdge**, y cambiando **paleta por mitad de pantalla** (arriba una paleta, abajo otra) con el copper para dar sensación de transición o de “borde suave”. Opcionalmente se aplica un blur real promediando píxeles con el blitter.

## Archivos clave

- `effects/blurred/blurred.c` — MakeCopperList (dos paletas: línea 127 vs 128), Init (CircleEdge + BlitterFill, buffer, carry), Render (pasadas de blur o solo swap de buffer).
- `data/blurred-pal-1.c`, `data/blurred-pal-2.c` — Dos paletas.
- `data/blurred-clip.c` — Máscara o forma adicional.

## Flujo

1. **Init**: Crea dos bitmaps de pantalla (doble buffer) y un buffer pequeño (SIZE×SIZE) más un “carry”. En cada bitmap de pantalla: dibuja un borde circular con **CircleEdge** y rellena con **BlitterFill**; luego copia el “clip” (máscara) al centro. La **copper list** carga una paleta hasta la línea 127 y **otra paleta** desde la línea 128 (CopWait en Y(127), dmacon off, LoadColors paleta 2, CopWait Y(128), dmacon on). Así la mitad superior e inferior tienen colores distintos y la forma parece “fundirse”. Activa copper y DMA.
2. **Render**: Si hay blur real: varias pasadas del blitter que promedian píxeles vecinos (por ejemplo copiar con desplazamiento y combinar). Si no, solo se intercambia el buffer activo y se actualizan bplpt. TaskWaitVBlank.
3. **Kill**: Libera bitmaps, buffer, carry y copper.

## Mecanismos de hardware que lo hacen posible

- **Copper**: Permite **cambiar la paleta a mitad de pantalla**. En la línea 127 se hace un WAIT y se cargan los colores de la paleta 2; en la línea 128 se vuelve a activar el DMA de raster. Así, sin cambiar un solo píxel del bitmap, la mitad superior se ve con una gama de colores y la inferior con otra; el cerebro lo interpreta como transición o desenfoque visual.
- **Blitter**: **CircleEdge** dibuja el contorno del círculo y **BlitterFill** rellena el interior (modo fill del blitter). Si se hace blur “de verdad”, el blitter puede copiar el bitmap desplazado y combinar con minterms (promedio) en varias pasadas.
- **DMA de raster**: Muestra los bitplanes. El “truco” de desactivar DMAF_RASTER un momento (entre líneas 127 y 128) hace que en esa franja no se dibuje nada (o se mantenga la línea anterior según el hardware); luego al cargar otra paleta el resto de la pantalla usa colores distintos.
- **Chip RAM**: Todos los bitmaps (screen, buffer, carry) están en chip para el blitter y el raster.

## Técnicas y trucos

- **Dos paletas en una pantalla**: El copper no está limitado a una sola paleta por frame; puede cargar COLOR00–COLOR15 tantas veces como se quiera, en distintas líneas. Aquí se usa para dar sensación de “gradiente” o de borde difuso entre dos looks.
- **CircleEdge + BlitterFill**: Forma suave sin dibujar píxel a píxel en CPU; el blitter hace el círculo y el relleno. La “suavidad” visual viene sobre todo de la paleta (colores intermedios) no del antialiasing.
- **Buffer pequeño**: Parte del efecto (la forma borrosa) puede calcularse en un bitmap reducido (SIZE×SIZE) y luego copiarse o escalarse a pantalla para ahorrar tiempo.

## Conceptos para principiantes

- **Paleta por línea** — En Amiga los registros de color (COLOR00, etc.) se pueden escribir en cualquier momento. Si los escribe el copper en una línea concreta (WAIT Y(128)), desde esa línea en adelante los píxeles se interpretan con los nuevos colores.
- **BlitterFill** — El blitter tiene un modo que rellena regiones delimitadas por los bits que ya hay en el plano (por ejemplo el contorno dibujado antes con CircleEdge). Muy rápido comparado con rellenar en CPU.
- **dmacon** — Registro que activa/desactiva canales DMA. Quitar DMAF_RASTER un instante hace que la Denise deje de leer bitplanes en esa zona; se usa aquí para “cortar” la pantalla y cambiar paleta sin afectar la parte ya dibujada.
