# Tutorial: GUI

## Objetivo

Una **interfaz gráfica** de prueba: botones, texto, tal vez un cuadro de diálogo. Usa la **libgui** del proyecto (font, dibujo de elementos) y responde a **eventos** de ratón o teclado (sistema de eventos del repo: input.device, mouse, keyboard).

## Archivos clave

- `effects/gui/gui.c` — Init (crea ventanas/botones con libgui, registra eventos), Render (dibuja la GUI o solo actualiza si hubo evento), Kill (libera GUI y desregistra eventos).
- `lib/libgui/gui.c`, `font.c` — Dibujo de controles y texto.
- `include/gui.h`, `include/font.h` — API de la GUI.

## Flujo

1. **Init**: Inicializa la **libgui** (o el subsistema de ventanas). Crea un bitmap para la pantalla y la copper. Dibuja los elementos de la GUI (marco, botones, etiquetas) en el bitmap usando las funciones de libgui (rectángulos, texto con la fuente). Registra un **manejador de eventos** (ratón: clic, movimiento; teclado: tecla pulsada) con el sistema (input.device o el driver de evento del repo). Activa copper y DMA.
2. **Render**: Comprueba si hay **eventos** pendientes (GetEvent o similar). Si el usuario hizo clic en un botón, se actualiza el estado (invertir color, ejecutar acción) y se **redibuja** esa zona del bitmap (o toda la GUI). Si no hay eventos, puede no hacer nada o solo TaskWaitVBlank. Actualiza bplpt si hubo doble buffer.
3. **Kill**: Desregistra el manejador de eventos. Libera elementos de la GUI, bitmap y copper.

## Mecanismos de hardware que lo hacen posible

- **CPU (68000)**: Ejecuta la **lógica de la GUI** (qué botón se pulsó, qué redibujar) y llama a las funciones de libgui para dibujar rectángulos, texto, etc. También **lee los eventos** del sistema (el driver de ratón/teclado escribe en una cola o en variables que la CPU lee).
- **Blitter**: La libgui puede usar el blitter para **rellenar** rectángulos (BlitterSetArea), **copiar** zonas (BitmapCopyArea) o **dibujar líneas** (BlitterLine) para los bordes de los botones. Sin blitter, dibujar cada píxel en CPU sería lento.
- **Copper**: Mantiene bplpt y paleta. La GUI se ve en el bitmap que la copper está mostrando.
- **DMA de raster**: Muestra el bitmap.
- **Sistema de entrada (Paula / CIA / custom)**: El **ratón** y el **teclado** están conectados al hardware (potgo, joy0dat, serdatr, o vía drivers que leen esos registros). El sistema (kernel o drivers) traduce eso en “eventos” (clic en (x,y), tecla X pulsada). La CPU no lee el hardware directamente en este efecto; usa la API de eventos.
- **Chip RAM**: Bitmap y datos de la GUI en chip.

## Técnicas y trucos

- **Doble buffer**: Para no parpadear al redibujar un botón, se dibuja en un buffer y se muestra el otro; o se redibuja solo el rectángulo del botón y se espera VBlank antes de cambiar bplpt.
- **Hit test**: Saber si el clic (mx, my) está dentro de un botón: (mx >= bx && mx < bx+bw && my >= by && my < by+bh). La CPU hace esta comparación cuando llega el evento “clic”.
- **Fuente**: libgui usa una fuente (bitmap de caracteres); dibujar texto = copiar los glifos al bitmap en la posición correcta (con blitter o CPU).

## Conceptos para principiantes

- **Evento** — Algo que ocurre “fuera” del bucle principal: el usuario pulsó un botón del ratón, movió el ratón, pulsó una tecla. El programa los recibe en forma de mensajes o de variables que el driver actualiza.
- **Hit test** — Comprobar si unas coordenadas (x,y) están dentro de un área (botón). Si sí, se ejecuta la acción del botón.
- **libgui** — Librería del proyecto que abstrae “dibujar un botón”, “dibujar texto”, etc. Por debajo usa el blitter y el bitmap. Así el efecto no programa el blitter a mano para cada control.
