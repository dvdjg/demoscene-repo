# Tutorial: Turmite (Hormiga de Turing)

## Objetivo

Efecto **Turmite**: una “hormiga” (autómata) que se mueve sobre una **cuadrícula** de celdas; en cada celda lee el **estado** (color o 0/1), aplica una **regla** (cambiar estado, girar, avanzar) y escribe el nuevo estado. Con reglas sencillas se generan patrones complejos. En pantalla se muestra la cuadrícula coloreada según el estado de cada celda; el **blitter** o la **CPU** actualizan los píxeles (o un buffer de “estado”) y se vuelca al bitmap visible.

## Archivos clave

- `effects/turmite/turmite.c` — Init (bitmap o buffer de estado, copper), Render (aplica la regla de la turmite N veces por frame: lee estado en (x,y), escribe nuevo estado, gira y avanza la hormiga; vuelca buffer a pantalla si es distinto).
- Regla: tabla (estado_actual, color_actual) -> (nuevo_estado, giro, nuevo_color). Ej. Langton's ant: negro->blanco, gira derecha; blanco->negro, gira izquierda.

## Flujo

1. **Init**: Crea un **buffer de estado** (una celda por pixel o una celda por bloque, ej. 64x64 celdas). Inicializa todas a 0 (o un color). Posición inicial de la hormiga (cx, cy) y dirección (0=arriba, 1=derecha, 2=abajo, 3=izquierda). Define la **regla** (qué hacer en cada estado). Crea bitmap de pantalla y copper; si el buffer es el mismo que el bitmap (1 pixel = 1 celda), no hace falta volcado. LoadColors. Activa DMA.
2. **Render**: Para cada paso (o N pasos por frame): **lee** estado = buffer[cx][cy]. **Escribe** nuevo estado según la regla (cambiar color en buffer/bitmap). **Gira** la dirección (derecha = (dir+1)%4, izquierda = (dir+3)%4). **Avanza**: cx += dx[dir], cy += dy[dir]; aplicar wrap o límites. Si el buffer no es el bitmap, **BlitterCopy** o vuelco del buffer al bitmap para mostrar. TaskWaitVBlank.
3. **Kill**: Libera buffer, bitmap y copper.

## Mecanismos de hardware que lo hacen posible

- **CPU**: Toda la **lógica de la turmite** (lectura de estado, tabla de reglas, escritura del nuevo estado, actualización de posición y dirección) es en CPU. Si cada celda es un byte o un píxel, la CPU escribe en el buffer. Con muchas celdas (ej. 320x256) y 1 pixel = 1 celda, escribir un píxel por paso es barato; el bitmap se va llenando con el patrón.
- **Blitter**: Si el “buffer de estado” es separado (ej. grid 64x64) y se **mapea a colores** para pantalla, se puede usar el blitter para **expandir** o **volcar** el buffer al bitmap (cada celda -> bloque de píxeles del mismo color). O para **limpiar** una zona. La mayoría del trabajo es la regla en CPU.
- **Copper**: bplpt y LoadColors. La paleta define los “estados” (colores). Si hay pocos estados (2–4), pocos planos; si hay más estados para patrones más ricos, más planos.
- **DMA de raster**: Muestra el bitmap donde se ha ido escribiendo el estado.
- **Chip RAM**: Buffer y bitmap en chip si el blitter los usa; si solo la CPU escribe, puede ser un buffer en chip para que al volcar (o si buffer = bitmap) la pantalla lo muestre.

## Técnicas y trucos

- **Langton's ant**: 2 estados (negro/blanco). En negro: escribe blanco, gira derecha. En blanco: escribe negro, gira izquierda. Después de miles de pasos aparece un “camino” caótico que termina en patrones recurrentes.
- **Varios pasos por frame**: Hacer 100–1000 pasos de la turmite por frame para que el patrón crezca rápido. Solo lectura/escritura de una celda por paso.
- **Colores**: Cada estado = un índice de color. La regla dice “pon color 3 aquí”; el bitmap tiene ese índice y LoadColors lo muestra. Hasta 16 o 32 estados (y colores) para patrones más vistosos.
- **Wrap**: (cx + dx + width) % width para que la hormiga salga por un lado y entre por el otro.

## Conceptos para principiantes

- **Turmite** — Autómata que camina sobre una cuadrícula; en cada celda lee el valor, lo cambia según una regla, gira y avanza. Es una variante de “hormiga de Langton” con más estados o reglas.
- **Regla (estado, entrada) -> (salida, giro)** — La “tabla” que define el comportamiento: si estoy en estado A y la celda tiene color X, escribo color Y, giro izquierda/derecha y avanzo. Con pocas reglas se generan patrones muy complejos.
- **Buffer de estado** — Array 2D que guarda el “color” o estado de cada celda. Puede ser el mismo bitmap (1 píxel = 1 celda) o un grid más pequeño que luego se dibuja ampliado.
