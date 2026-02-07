# Tutorial: TextScroll

## Objetivo

**Scroll de texto** en pantalla: las líneas de texto suben (o bajan) y se dibujan con una fuente 8×8. Cada línea de la pantalla puede mostrar una fila distinta del buffer de texto; el “scroll” se simula **cambiando los punteros de bitplanes por línea** en el copper (cada línea apunta a una fila distinta del buffer grande).

## Archivos clave

- `effects/textscroll/textscroll.c` — MakeCopperList (bplpt por línea), SetupLinePointers, RenderLine, RenderNextLineIfNeeded.
- `data/text-scroll-font.c` — Fuente 8×8 (bitmap de caracteres).
- `data/text-scroll.txt` — Texto a mostrar.
- `data/background.c` — Fondo (segundo playfield o planos extra).

## Flujo

1. **Init**: crea un bitmap “scroll” más alto que la pantalla (p. ej. HEIGHT+16) y 1 bitplane para el texto; el otro plano puede ser un fondo estático. Rellena linebpl: array de punteros a instrucciones copper (cada una es un MOVE de bplpt para una línea Y). Construye dos copper lists; en cada una, **por cada Y** hay un WAIT y un CopMove32(bplpt[0], ptr) — el ptr se actualizará en Render. Configura dual playfield, LoadColors para fuente y fondo, activa DMA.
2. **Render**:  
   - **SetupLinePointers**: según frameCount (y SPEED), calcula qué “línea lógica” del buffer corresponde a la parte superior de la pantalla. Para cada Y de pantalla, el puntero que se escribe en la copper es `start + stride*y`, dando la vuelta al buffer si hace falta. Así, sin copiar memoria, la pantalla “muestra” una ventana deslizante sobre el buffer.  
   - **RenderNextLineIfNeeded**: cada cierto número de frames (cada 8 si SPEED=1), avanza una “línea” de texto. Borra la franja donde se va a dibujar, llama a RenderLine(ptr, line_start, size). RenderLine dibuja un carácter 8×8 por cada carácter de la cadena: indexa en la fuente por (carácter-32), copia 8 filas del glifo al bitmap (dst, src, stride).  
   - CopListRun(cp[active]), TaskWaitVBlank(), alternar active.
3. **Kill**: para copper y blitter, libera listas, bitmap y linebpl.

## Técnicas y trucos

- **Scroll sin mover píxeles**: en lugar de copiar todo el bitmap hacia arriba/abajo, el buffer es “circular” en Y. Cada línea de pantalla apunta a una fila del buffer (ptr = base + (offset + y) % height * stride). Cambiando offset cada frame, el contenido parece desplazarse; solo hay que rellenar la nueva línea que “entra” (RenderNextLineIfNeeded).
- **Copper: un MOVE de bplpt por línea**: la lista tiene 256 (o HEIGHT) pares de instrucciones, cada una con el puntero al inicio de esa línea del bitmap. Modificar esos punteros (CopInsSet32) cada frame es la base del scroll suave.
- **Fuente 8×8**: un bitmap donde cada carácter es un bloque 8×8; el índice es (ascii - 32). Dibujar un carácter = copiar 8 bytes (o 8 palabras) desde font.planes[0] + index*ancho_fuente al destino con stride = bytesPerRow del scroll. Puede hacerse con blitter (BitmapCopyMasked) o en CPU.
- **Dual playfield**: texto en un playfield, fondo en otro; cada uno con su paleta (LoadColors para 0 y para 8).

## Conceptos para principiantes

- **bplpt (BPLxPT)** — Registros que contienen la dirección de inicio de cada plano de bits. Si cambias bplpt por línea (vía copper), cada línea puede leer sus datos de una dirección distinta = scroll vertical “por línea” sin mover datos.
- **Modulo (bpl1mod, bpl2mod)** — Número de bytes que el hardware suma al puntero de bitplanes al pasar a la siguiente línea. Normalmente = bytesPerRow (o 0 si interleaved). Valores negativos hacen que la “siguiente” línea sea la misma (line doubling).
- **Stride** — bytesPerRow del bitmap; distancia en memoria entre el inicio de una línea y la siguiente.
