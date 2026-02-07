# Tutorial: Thunders (Rayos)

## Objetivo

Efecto de **rayos** (relámpagos): líneas **zigzagueantes** o **ramificadas** que aparecen de forma aleatoria o rítmica, con **flash** de pantalla (cambio brusco de color o de luminosidad). Se dibujan con **CpuLine** o **BlitterLine** (varios segmentos por rayo) y el “flash” se puede hacer con **copper** (cambiar temporalmente LoadColors a blanco o muy claro) o con **blitter** (relleno rápido de todo el bitmap con color claro).

## Archivos clave

- `effects/thunders/thunders.c` — Init (bitmap, copper), Render (genera posición y forma del rayo — segmentos con ángulo aleatorio—, dibuja líneas; opcional flash con LoadColors o relleno).
- Posible tabla de segmentos o generación procedural (random).

## Flujo

1. **Init**: Crea bitmap y copper. Inicializa generador aleatorio o tabla de “formas” de rayo. LoadColors: paleta con blanco/amarillo para el rayo y colores normales para el fondo. Activa DMA.
2. **Render**: Decide si este frame hay rayo (por tiempo o aleatorio). Si hay rayo: **genera la trayectoria** (por ejemplo inicio arriba, cada segmento con un pequeño ángulo aleatorio respecto al anterior, hasta abajo o un lado). **Dibuja** cada segmento con **CpuLine** o **BlitterLine**. Opcional: **flash** — durante 1–2 frames cambia LoadColors a pantalla muy clara (o rellena con BlitterFill color blanco) y luego restaura. Limpia el rayo del frame anterior si no se quiere estela. TaskWaitVBlank.
3. **Kill**: Libera bitmap y copper.

## Mecanismos de hardware que lo hacen posible

- **Blitter**: **BlitterLine** para cada segmento del rayo (rápido). Para el **flash**: **BlitterFill** de todo el bitmap con el color “blanco” (o el índice que sea) en muy pocas pasadas (por plano). Así toda la pantalla parpadea sin que la CPU tenga que escribir cada píxel.
- **Copper**: **LoadColors** puede usarse para el flash: en la copper list, en una línea concreta (o en todo el frame si se cambia al inicio) cargar una paleta donde todos los colores son blanco o amarillo claro; al frame siguiente restaurar la paleta normal. El efecto es que “todo” brilla porque los mismos índices de píxel pasan a ser blancos.
- **CPU**: **Generación del rayo** (lista de puntos: x0,y0, x1,y1, ... con variación aleatoria). Llamadas a CpuLine o preparación de parámetros para BlitterLine.
- **DMA de raster**: Muestra el bitmap (y aplica la paleta que la copper haya cargado; por eso el flash por paleta funciona).
- **Chip RAM**: Bitmap en chip.

## Técnicas y trucos

- **Rayo procedural**: Empezar en (cx, 0). Para i = 1..N: siguiente_x = anterior_x + random(-step, +step), siguiente_y = anterior_y + segment_length. Así se obtiene un camino que “baja” con zigzag. Varios segmentos cortos dan aspecto de rayo.
- **Flash por paleta**: Guardar copia de la paleta normal. Cuando hay rayo: durante 1 frame escribir en LoadColors paleta “flash” (todos los colores = blanco o amarillo). Al siguiente frame restaurar. Muy barato y muy efectivo.
- **Ramas**: Un rayo puede “ramificarse”: en un punto intermedio, crear un segundo rayo con ángulo distinto que va hacia otro lado. Dibujar ambos con líneas.
- **Doble imagen**: Dibujar el rayo dos veces con ligero desplazamiento (o en dos planos) puede dar efecto de “brillo” o grosor.

## Conceptos para principiantes

- **Flash (pantalla)** — Cambio muy breve de toda la pantalla a un color claro (blanco/amarillo) para simular el destello del relámpago. Se hace cambiando la paleta o rellenando todo el bitmap.
- **Segmentos de rayo** — Un rayo no es una línea recta; es una secuencia de segmentos con pequeños ángulos aleatorios. Así se imita la forma irregular del relámpago.
- **LoadColors para efecto** — Usar la copper para cambiar todos los colores de la paleta de golpe permite efectos globales (flash, fade) sin tocar el contenido del bitmap.
