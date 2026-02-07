# Tutorial: Kbtest

## Objetivo

**Prueba de teclado**: mostrar en pantalla qué tecla se ha pulsado (código de tecla o carácter). Sirve para verificar que el driver de teclado y el sistema de eventos funcionan y para depurar controles en otros efectos.

## Archivos clave

- `effects/kbtest/kbtest.c` — Init (bitmap, copper, tal vez registro con el driver de teclado), Render (lee evento de tecla, dibuja el código o carácter en el bitmap, o actualiza un texto).
- Sistema: input.device, keyboard.device o el mecanismo de eventos del repo (system/event, system/keyboard).

## Flujo

1. **Init**: Crea bitmap y copper. Puede registrar un manejador de teclado o abrir el dispositivo de teclado. Limpia la pantalla o dibuja un mensaje “Pulsa una tecla”. Activa DMA.
2. **Render**: Comprueba si hay una **tecla pulsada** (leyendo una cola de eventos o una variable global que el driver de teclado actualiza). Si hay tecla: la convierte a carácter o muestra su código (número) y **dibuja** ese texto en el bitmap (usando una fuente o convirtiendo a hexadecimal). Si no hay tecla, solo TaskWaitVBlank. Opcionalmente se borra la zona de texto antes de redibujar.
3. **Kill**: Cierra el dispositivo o desregistra el manejador. Libera bitmap y copper.

## Mecanismos de hardware que lo hacen posible

- **Teclado (hardware)**: El teclado del Amiga está conectado a **Paula** (o a un CIA) por un protocolo serie. Cada tecla pulsada o soltada envía códigos (scancodes). El **driver de teclado** (en el sistema del repo) lee esos códigos y los convierte en “eventos” o en caracteres.
- **CPU (68000)**: Lee los eventos o el buffer del driver (por ejemplo “última tecla = X”). Convierte el código a texto (por ejemplo número a hexadecimal con sprintf o una tabla) y decide **dónde** dibujar en el bitmap.
- **Blitter** (o CPU): **Dibuja el texto** en el bitmap: cada carácter es un glifo de la fuente; se copia en la posición (x, y). Puede hacerse con BitmapCopy de glifos o con una rutina que escribe píxel a píxel.
- **Copper**: Mantiene bplpt y paleta. La pantalla muestra el bitmap donde hemos escrito el código de la tecla.
- **DMA de raster**: Muestra el bitmap.
- **Chip RAM**: Bitmap y fuente en chip.

## Técnicas y trucos

- **No bloquear**: El efecto no debe “esperar” a una tecla en un bucle cerrado; debe comprobar “¿hay tecla?” cada frame y, si la hay, mostrarla. Así el resto del sistema (y la pantalla) sigue activo.
- **Mostrar código raw**: Si se muestra el scancode (número), se puede ver qué envía el teclado realmente; útil para mapear teclas raras o para depurar.
- **Fuente pequeña**: Una fuente 8×8 o 8×16 permite mostrar “0x2A” o “Key: 42” en poco espacio.

## Conceptos para principiantes

- **Scancode** — Código numérico que envía el teclado por cada tecla (pulsada o soltada). No es el carácter (eso depende del mapa de teclado y de mayúsculas). En kbtest a veces se muestra el scancode para depuración.
- **Driver de teclado** — Parte del sistema que habla con el hardware del teclado (registros, interrupciones) y ofrece a los programas “eventos” o “última tecla”. El efecto solo usa esa API, no toca el hardware directamente.
- **Evento** — Mensaje que dice “tecla X pulsada” o “tecla X soltada”. El programa los lee en el bucle de Render y reacciona (por ejemplo actualizando el texto en pantalla).
