# Tutorial: ShowPCHG (Mostrar PCHG)

## Objetivo

Demostrar o visualizar el uso de **PCHG** (Palette Change) del Amiga: cambio de **paleta por línea** usando la copper. PCHG es una **estructura de datos** que la copper puede seguir para ejecutar **LoadColors** (y opcionalmente otras instrucciones) en líneas concretas, sin tener que escribir toda la copper list a mano. El efecto muestra una pantalla donde los colores **cambian por línea** (degradados, bandas, etc.) usando PCHG.

## Archivos clave

- `effects/showpchg/showpchg.c` — Init (crea o carga datos PCHG, configura copper para usar PCHG), Render (puede actualizar datos PCHG para animación, o solo VBlank).
- Documentación o código de **PCHG**: formato de la estructura (lista de (línea, valores de color)) que la copper interpreta.

## Flujo

1. **Init**: Crea bitmap y copper. **Construye la estructura PCHG**: lista de cambios de paleta, cada uno con número de línea y los 32 bytes de color (o los que se quieran cambiar). La copper list debe incluir la **instrucción que activa PCHG** (cop1lc o similar, según implementación) apuntando a la estructura PCHG en chip RAM. Carga el bitmap si hay gráficos. Activa DMA.
2. **Render**: Si los colores se animan: actualiza los valores en la estructura PCHG (por ejemplo desplaza un degradado, color cycling). La copper ya está configurada para leer PCHG; en el siguiente frame el raster usará los nuevos valores. TaskWaitVBlank.
3. **Kill**: Libera PCHG, bitmap y copper.

## Mecanismos de hardware que lo hacen posible

- **Copper**: El Amiga permite que la copper **siga una lista enlazada** de instrucciones: en cada línea (o cuando toca) la copper lee la **siguiente entrada** de la lista y ejecuta **LoadColors** (o otras órdenes). Eso es el mecanismo **PCHG**: la lista está en chip RAM y contiene (WAIT línea X, luego MOVEM para COLOR00–COLOR31). Así se pueden tener **muchos cambios de paleta por pantalla** sin una copper list gigante estática; la lista se genera o se actualiza en software.
- **CPU**: Genera o **actualiza la estructura PCHG** (qué línea, qué colores). Si hay animación, cada frame se modifican los valores de color en la estructura; la copper los aplica al dibujar.
- **DMA de raster**: Lee el bitmap; los colores con los que se muestran los píxeles son los que LoadColors dejó activos para esa línea (según PCHG).
- **Chip RAM**: La estructura PCHG debe estar en **chip RAM** porque la copper la lee por DMA.

## Técnicas y trucos

- **Formato PCHG**: Suele ser una secuencia de (WAIT, valor) que espera a una línea y luego escribe en registros de color. Cada “nodo” apunta al siguiente. Al terminar se apunta a sí mismo o a una instrucción NOP.
- **Degradados**: Rellenar la estructura PCHG con colores que van de A a B según la línea (interpolar). Así se tiene un degradado vertical “real” (cada línea con su color) sin usar más planos de bit.
- **Animación**: Cambiar los valores RGB en la estructura PCHG cada frame (por ejemplo rotar un degradado) da animación de color sin tocar el bitmap.

## Conceptos para principiantes

- **PCHG (Palette Change)** — Mecanismo del Amiga para cambiar la paleta en varias líneas usando una lista que la copper sigue. La lista contiene “en la línea X, carga estos colores”. Ahorra tener que poner cientos de WAIT+LoadColors a mano en la copper.
- **Copper list enlazada** — La copper puede ejecutar una instrucción que dice “salta a esta otra dirección”; así la lista de órdenes puede estar en cualquier parte de la memoria (chip) y ser dinámica.
- **LoadColors** — Instrucción de la copper que escribe en los registros COLOR00–COLOR31 (o un rango). Con PCHG se hace muchas veces por frame, cada una en una línea distinta.
