# Tutorial: Fire (Fire RGB)

## Objetivo

Efecto de **fuego** en colores (RGB). Se mantiene un buffer 2D donde cada “celda” tiene una temperatura; cada frame se suaviza y se sube el fuego, se dibuja en un buffer **chunky** (un byte/word por píxel) y luego se convierte a **planar** con el blitter para mostrarlo en pantalla. En versiones AGA puede usarse HAM u otros modos.

## Archivos clave

- `effects/fire-rgb/fire-rgb.c` — Init (buffers chunky, planar, fire), copper (cuadruplicado de línea), Render (física del fuego + c2p), ChunkyToPlanar (blitter).
- `data/dualtab.c` — Tabla de colores/paleta para el fuego.
- `data/gen-dualtab.py` — Script que genera esas tablas.

## Flujo

1. **Init**: reserva dos buffers chunky y un buffer “fire” (estado de temperatura), dos bitmaps para pantalla (doble buffer). Construye copper list que hace **line doubling/quad** (modulo negativo para repetir la misma línea varias veces) y escribe BPLCON1 por línea para un efecto de “pixelado” o alineación. En Render se usa el blitter para convertir chunky→planar en varias pasadas.
2. **Render**: (resumido) actualiza el buffer de fuego (difusión hacia arriba, fuentes de calor abajo), mapea ese buffer a colores en el buffer chunky activo. Luego inicia la conversión **chunky-to-planar** con el blitter; esta conversión suele hacerse en **varias fases** (por ejemplo 8×4 swap, luego separación a planos). El blitter puede dispararse y completarse en la misma frame o repartirse; a veces se usa interrupción de blitter para encadenar la siguiente fase. Al final se actualizan los punteros bplpt en la copper para mostrar el buffer ya convertido.
3. **Kill**: libera buffers y listas.

## Técnicas y trucos

- **Fuego como campo de valores**: el “fire” es un array 2D; cada celda = temperatura. Cada frame: se promedian vecinos (difusión), se desplaza hacia arriba y se inyectan valores altos en la base. Luego se mapea temperatura → color (tabla) y se escribe en el buffer chunky.
- **Chunky-to-planar con blitter**: el Amiga dibuja por planos de bits, no por “bytes por píxel”. Para tener un buffer en formato chunky (fácil de escribir desde C) hay que convertirlo a planar. El efecto hace varias pasadas de blitter: primero reordena palabras (shift + minterms para juntar bits), luego separa en los 4 (o 6) planos. Cada pasada configura bltapt, bltbpt, bltdpt, bltcon0/1, bltsize; a veces se usa BLITREVERSE para dirección inversa.
- **Line quadrupling**: en la copper, se pone bpl1mod/bpl2mod de modo que en 3 de cada 4 líneas el “modulo” sea negativo y la Denise repita la misma línea de datos; así 64 líneas lógicas se convierten en 256 visibles sin duplicar memoria.
- **Interrupción de blitter**: si la conversión es larga, se puede dividir en pasos y en la IRQ del blitter iniciar el siguiente paso (ChunkyToPlanar en el código usa un `phase` y posiblemente ClearIRQ(INTF_BLIT)).
- **Doble buffer**: un chunky se muestra (ya convertido a planar) mientras el otro se rellena con el nuevo frame de fuego.

## Conceptos para principiantes

- **Chunky** — Formato donde cada píxel es un byte (o word) consecutivo en memoria. Fácil de indexar: `buf[y*width+x] = color`.
- **Planar** — Formato Amiga: varios planos de bits; el bit i del píxel (x,y) está en plano i. Para escribir un color hay que tocar todos los planos. El blitter puede hacer operaciones por plano muy rápido.
- **c2p (chunky to planar)** — Conversión entre ambos formatos. Suele hacerse con desplazamientos (ASHIFT), máscaras y minterms del blitter para no hacerlo en CPU.
- **BPLCON1** — Registro que controla el scroll horizontal (fine scroll) de los planos. Usarlo por línea en copper permite “desplazar” píxeles sin mover memoria.
