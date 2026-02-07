# Tutorial: Glitch

## Objetivo

Efecto **glitch**: corrupción visual voluntaria. Se escribe en registros del hardware (color, BPLCON1, o en zonas del bitmap) valores **aleatorios** o inesperados durante unos frames, para que la imagen “rompa” o parpadee de forma estética.

## Archivos clave

- `effects/glitch/glitch.c` — Init (bitmap normal, copper), Render (en algunos frames: escribe valores aleatorios en custom->color[], o en bplcon1, o en una zona del bitmap; luego restaura).
- Puede usar random() o una secuencia pseudoaleatoria.

## Flujo

1. **Init**: Crea bitmap con una imagen “normal” (o la carga). Configura copper. Todo como un efecto estándar.
2. **Render**: La mayor parte del tiempo muestra la imagen normal. Cada N frames (o con cierta probabilidad) activa un “glitch”: por ejemplo, durante 1–3 frames escribe en **custom->color[0]** a **custom->color[15]** valores aleatorios (random() & 0xfff), o escribe en **custom->bplcon1** un valor que desplaza los planos, o escribe bytes aleatorios en una franja del bitmap. En el frame siguiente restaura los colores o el bitmap desde una copia. TaskWaitVBlank.
3. **Kill**: Libera recursos.

## Mecanismos de hardware que lo hacen posible

- **Registros del custom (CPU)**: La **CPU** escribe directamente en los registros del chip (custom->color[], custom->bplcon1, etc.). Esos registros controlan en tiempo real qué colores ve la pantalla y cómo se leen los bitplanes. Escribir valores “raros” produce colores incorrectos o desplazamiento horizontal (glitch de “tearing”).
- **Copper**: En modo normal la copper carga la paleta; si la CPU escribe encima entre medias, durante ese frame se ve la corrupción. El glitch puede ser “escribir encima de lo que puso el copper”.
- **DMA de raster**: Sigue leyendo el bitmap; si el bitmap se corrompe (CPU escribe bytes aleatorios en una zona), esa zona se ve mal.
- **Chip RAM**: Si se corrompe una zona de chip (el bitmap), el raster la muestra tal cual en el siguiente refresco.
- **Blitter** (opcional): Podría usarse para “copiar” una franja desplazada (simular tearing) en lugar de escribir aleatorio.

## Técnicas y trucos

- **Glitch corto**: Solo 1–2 frames de corrupción; luego se restaura desde una copia de la paleta o del bitmap. Si no, la imagen quedaría rota para siempre.
- **Restaurar desde copia**: Se guarda una copia de la paleta (o del bitmap) en Init; cuando termina el glitch se hace memcpy o LoadColors desde esa copia.
- **Aleatoriedad**: random() o un LFSR (generador pseudoaleatorio) para que no sea siempre el mismo glitch. Se puede limitar a ciertos registros (solo colores, o solo bplcon1) para controlar el tipo de efecto.
- **BPLCON1**: Cambiarlo desplaza los bitplanes horizontalmente; un valor “malo” produce líneas desplazadas = “tearing” clásico de glitch.

## Conceptos para principiantes

- **custom** — Puntero a la estructura que mapea los registros del hardware. Escribir en custom->color[i] cambia de inmediato el color que la Denise usa para el índice i.
- **Glitch** — En electrónica/computación, un “glitch” es un fallo momentáneo. En demos se usa a propósito para un efecto visual de “rotura” o de señal inestable.
- **BPLCON1** — Controla scroll horizontal fino. Un valor cambiado hace que esa línea (o todo el playfield) se lea desplazada, lo que se ve como una “ruptura” en la imagen.
