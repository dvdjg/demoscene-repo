# Tutorial: PlayAHX (Reproducción AHX)

## Objetivo

Reproducir módulos o sonidos en formato **AHX** (un formato de música tracker para Amiga). El efecto consiste en **cargar un archivo AHX** y usar la rutina de **reproducción** (driver) que actualiza el **audio** en cada frame o en cada tick, mientras en pantalla puede mostrarse una visualización o simplemente la demo.

## Archivos clave

- `effects/playahx/playahx.c` — Init (abre y carga el módulo AHX, inicializa el driver AHX), Render (llama al reproductor AHX para que avance el tiempo y envíe muestras a Paula), posible visualización en pantalla.
- Driver AHX: código que interpreta el módulo y escribe en los **registros de Paula** (AUDxDAT, AUDxVOL, etc.).

## Flujo

1. **Init**: Carga el archivo AHX desde disco o desde datos embebidos. **Inicializa el driver AHX** (tablas de notas, efectos, canales). Configura pantalla si hay visualización (bitmap, copper). Opcional: precalcula datos para un scope o spectrum.
2. **Render**: Cada frame: **PlayAHX()** o equivalente — el driver lee el patrón actual, aplica efectos (vibrato, slide, etc.) y **escribe en Paula** (frecuencia, volumen, forma de onda o puntero a muestra). La pantalla puede mostrar un scope (muestras de audio) o un espectro. TaskWaitVBlank.
3. **Kill**: Detiene el reproductor (silencia canales), cierra el archivo, libera memoria y copper.

## Mecanismos de hardware que lo hacen posible

- **Paula (chip de audio)**: Tiene **4 canales** de audio (8 bits, DMA de audio). Cada canal tiene: **AUDxLCH/LCL** (puntero a la muestra en chip RAM), **AUDxLEN** (longitud en palabras), **AUDxPER** (periodo = frecuencia), **AUDxVOL** (volumen 0–64). El driver AHX pone en estos registros los valores que corresponden a la nota y el sample actual; Paula reproduce sin que la CPU tenga que enviar muestras en tiempo real.
- **CPU**: El **driver AHX** corre en la CPU: interpreta el módulo (patrones, notas, efectos), calcula la frecuencia (periodo) a partir de la nota, actualiza punteros a samples y escribe en los registros de Paula. Suele hacerse una vez por frame o cada “tick”.
- **Chip RAM**: Las muestras de audio (waveforms) deben estar en **chip RAM** porque Paula las lee por DMA. El módulo AHX se carga en memoria y las muestras se copian a buffers en chip.
- **Copper / Blitter**: Solo si hay visualización (scope, fondo); el sonido en sí no los usa.

## Técnicas y trucos

- **Tick vs frame**: Algunos reproductores avanzan “ticks” (por ejemplo 50 ticks por segundo) independientes del frame; así la música no acelera o ralentiza con el framerate. Se puede usar un timer o contar VBlanks.
- **Doble buffer de samples**: Si se modifican los samples (por ejemplo para efectos), usar dos buffers y alternar; Paula lee uno mientras la CPU rellena el otro.
- **Visualización**: Leer los registros de audio (o un buffer de muestras) para dibujar un oscilloscope o barras de volumen en el bitmap con el blitter o la CPU.

## Conceptos para principiantes

- **AHX** — Formato de música tracker para Amiga. Contiene patrones (qué nota suena en cada canal en cada fila), samples (waveforms) y efectos. El “driver” es el código que lee ese formato y controla Paula.
- **Paula** — Chip de sonido del Amiga. Reproduce 4 canales en paralelo leyendo muestras desde chip RAM por DMA. La CPU solo debe actualizar los registros (puntero, longitud, periodo, volumen) cuando cambia la nota o el efecto.
- **Periodo (AUDxPER)** — Valor que define la frecuencia de reproducción: periodo más bajo = tono más alto. Se calcula a partir de la nota musical (ej. tabla de periodos para cada nota).
