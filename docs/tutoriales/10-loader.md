# Tutorial: Loader

## Objetivo

Pantalla de **carga** con una barra de progreso que se va rellenando y **música** de Protracker. La barra es un rectángulo dibujado con **líneas por CPU** (CpuLine); el avance depende del frameCount (simulado aquí; en una demo real podría depender de carga de datos).

## Archivos clave

- `effects/loader/loader.c` — Init (bitmap, copper, CpuLine para el marco, CopSetupBitplanes, copia del bitmap “loader” estático), Render (avanza x y dibuja líneas verticales dentro del marco), Kill.
- `data/loader.c` — Bitmap y paleta del dibujo estático de la barra (marco, fondo).
- `include/line.h` — CpuLineSetup, CpuLine.
- Reproducción: PtInstallCIA, PtInit(LoaderModule, LoaderSamples), PtEnable; PtEnd, PtRemoveCIA en Kill.

## Flujo

1. **Init**: instala el driver de tiempo (CIA) para Protracker, inicializa el módulo y samples, activa reproducción. Crea bitmap y copper list. **CpuLineSetup(screen, 0)** prepara el dibujo de líneas en el plano 0. Dibuja el **marco** del recuadro de la barra con 4 CpuLine (arriba, abajo, izquierda, derecha). SetupPlayfield, LoadColors. Habilita blitter, **BitmapCopy** del bitmap prehecho (loader) a la pantalla, WaitBlitter, desactiva blitter. Añade a la copper los MOVE de bitplanes, CopListFinish, CopListActivate, activa DMA de raster.
2. **Render**: calcula newX = frameCount>>3 (limitado a 121). En un bucle, dibuja líneas verticales desde (X1+x, Y1) hasta (X1+x, Y2) con CpuLine para cada x desde el último hasta newX. Así la barra “crece” hacia la derecha frame a frame.
3. **Kill**: BlitterStop, CopperStop, borra lista y bitmap; PtEnd, PtRemoveCIA, desactiva DMA de audio.

## Técnicas y trucos

- **CpuLine** — Dibujo de segmentos con la CPU (típicamente Bresenham). Más lento que el blitter pero suficiente para un borde y unas pocas líneas por frame. CpuLineSetup(screen, plane) fija el bitmap y el plano; luego cada CpuLine(x1,y1,x2,y2) dibuja un segmento.
- **Barra = muchas líneas verticales**: en lugar de rellenar un rectángulo con blitter, se dibuja una línea vertical por cada “columna” del avance. Con frameCount creciente, cada frame se añaden unas cuantas columnas.
- **Imagen estática de fondo**: el aspecto “bonito” del loader (marco, logo) viene de un bitmap pregenerado (loader.c); se copia una vez en Init con BitmapCopy. La barra se pinta encima en el mismo plano (o en uno que se combine).
- **Música en segundo plano**: Protracker usa la CIA para el tempo y Paula para el audio; una vez PtInit y PtEnable, la música suena sin hacer nada más en Render. Útil para cargar el siguiente efecto mientras se muestra esta pantalla.

## Conceptos para principiantes

- **CpuLineSetup / CpuLine** — API de libgfx para dibujar líneas con la CPU. Necesita el bitmap y el plano; internamente usa algoritmo de línea (Bresenham) y escribe en los planos.
- **BitmapCopy** — Copia todo un bitmap (o área) a otra posición con el blitter; en Init se usa para “pegar” el gráfico del loader de una vez.
- **Protracker (Pt*)** — Librería para reproducir módulos .mod; requiere instalar un manejador de interrupción (CIA) para el timing y pasar el módulo y samples. Ver libpt, ptplayer.asm.
