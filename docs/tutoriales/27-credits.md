# Tutorial: Credits

## Objetivo

Pantalla de **créditos**: texto o nombres que se muestran estáticos o en scroll. Suele ser un bitmap con el texto ya dibujado (o se dibuja con una fuente) y se muestra con la copper; opcionalmente hay scroll vertical u horizontal.

## Archivos clave

- `effects/credits/credits.c` — Init (bitmap con texto o carga de fuentes), Render (opcional scroll o cambio de “página” de créditos).
- Datos: bitmap de créditos o fuente + cadenas de texto.

## Flujo

1. **Init**: Crea bitmap y lo rellena con el texto de créditos (carácter a carácter con una fuente, o copia un bitmap pregenerado). Configura playfield, paleta y copper (CopSetupBitplanes). Si hay varias “páginas”, pueden estar en regiones distintas del mismo bitmap.
2. **Render**: Si es estático, no hace falta hacer nada (o solo TaskWaitVBlank). Si hay scroll, se actualizan los punteros bplpt por línea (como en TextScroll) o el modulo (bpl1mod) para desplazar. Si hay varias páginas, cada N frames se cambia qué región del bitmap se muestra (cambiando bplpt).
3. **Kill**: Libera bitmap y copper.

## Mecanismos de hardware que lo hacen posible

- **Copper**: Mantiene **bplpt** (qué parte de la memoria se muestra). Para scroll vertical “suave”, la copper puede tener un MOVE de bplpt por línea, apuntando cada línea a una fila distinta del buffer grande (igual que en TextScroll). Para scroll por “línea completa” se puede cambiar solo el puntero inicial (bplpt) al inicio del frame.
- **Blitter**: Si el texto se dibuja en tiempo real (no pregenerado), el blitter puede **BitmapCopy** de cada glifo de la fuente al bitmap en la posición correcta. Más rápido que escribir píxel a píxel en CPU.
- **DMA de raster**: Lee el bitmap desde chip RAM y lo envía a la Denise. El contenido del bitmap (texto) es lo que ve el usuario.
- **CPU**: Si se dibuja texto con una fuente, la CPU (o una rutina que usa el blitter) coloca cada carácter; si el bitmap viene pregenerado, la CPU solo controla cuándo cambiar de página o scroll.
- **Chip RAM**: Bitmap de créditos y posible buffer de fuente en chip.

## Técnicas y trucos

- **Bitmap pregenerado**: La forma más simple es tener un .c con el bitmap ya relleno (generado por script desde un texto o imagen). Init solo copia ese bitmap a pantalla o lo usa directamente como bplpt.
- **Scroll con bplpt por línea**: Igual que en TextScroll: un buffer más alto que la pantalla; cada línea de pantalla apunta a una fila del buffer; al cambiar el “offset” cada frame, el texto parece subir o bajar.
- **Varias páginas**: Varios créditos en el mismo bitmap (uno debajo de otro); cambiar bplpt para que apunte al inicio de la página N hace “cambiar de slide” sin redibujar.

## Conceptos para principiantes

- **bplpt** — Puntero al inicio de cada plano de bits. Si el bitmap es más grande que la pantalla, cambiar bplpt permite “ventana deslizante” sin copiar memoria.
- **Scroll suave** — En lugar de saltar de a un carácter o de a una línea, se desplaza de a un píxel (o de a unas líneas) usando bplpt por línea o modulo. Da sensación de movimiento continuo.
- **Fuente (font)** — Bitmap donde cada carácter es un bloque (p. ej. 8×8). Dibujar texto = copiar el bloque correspondiente a cada carácter en la posición (x, y) del bitmap de destino.
