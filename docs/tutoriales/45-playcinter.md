# Tutorial: PlayCinter (Reproducción Cinter)

## Objetivo

Reproducir música o sonidos con el **driver Cinter** (reproductor de módulos para Amiga). Similar a PlayAHX y PlayProtracker: se carga un módulo (formato soportado por Cinter) y el **driver** actualiza los registros de **Paula** para que el sonido salga por los 4 canales.

## Archivos clave

- `effects/playcinter/playcinter.c` — Init (carga módulo, inicializa driver Cinter), Render (llama al reproductor cada frame/tick), posible visualización.
- Código del driver Cinter: inicialización, avance de patrón, cálculo de periodo/volumen y escritura en registros AUDx*.

## Flujo

1. **Init**: Carga el archivo del módulo (desde disco o datos). Inicializa el driver Cinter (punteros a patrones, samples, velocidad, BPM). Configura pantalla si la hay. Los samples del módulo deben estar (o copiarse) en chip RAM.
2. **Render**: Cada frame: **Cinter_Play()** o similar — el driver avanza el patrón (filas), aplica efectos (arpeggio, slide, vibrato, etc.) y escribe en **Paula** (AUDxLCH/LCL, AUDxLEN, AUDxPER, AUDxVOL). Opcional: dibujar scope o nivel de canales. TaskWaitVBlank.
3. **Kill**: Silencia canales (volumen a 0 o detener DMA de audio), libera memoria del módulo y recursos de pantalla.

## Mecanismos de hardware que lo hacen posible

- **Paula**: Los **4 canales** de audio se controlan con los registros AUDx*. El driver Cinter pone en **AUDxLCH/LCL** el puntero al sample en chip RAM, en **AUDxLEN** la longitud, en **AUDxPER** el periodo (frecuencia) y en **AUDxVOL** el volumen. Paula reproduce en DMA; la CPU no envía las muestras en tiempo real.
- **CPU**: El driver Cinter corre en la CPU: interpreta el formato del módulo (patrones, notas, efectos), calcula periodos a partir de las notas (tabla de periodos) y escribe en los registros de Paula. Se ejecuta una vez por frame o por tick.
- **Chip RAM**: Samples en chip RAM para que Paula pueda leerlos por DMA.
- **Copper / Blitter**: Solo para la parte visual si existe (fondo, scope); el audio no los usa.

## Técnicas y trucos

- **Compatibilidad de formato**: Cinter puede soportar un subconjunto de Protracker o un formato propio. Revisar la documentación del driver para saber qué efectos (E0x, E1x, etc.) están implementados.
- **Prioridad de interrupciones**: Si se usa el reproductor en una interrupción (por ejemplo vertical blank), hay que asegurarse de no usar demasiado tiempo en el handler para no perder frames.
- **Visualización**: Leer AUDxDAT o un buffer de muestras para dibujar formas de onda o niveles en pantalla.

## Conceptos para principiantes

- **Cinter** — Driver (reproductor) de módulos de música para Amiga. Lee el formato del módulo y controla Paula para que suene la música.
- **Paula (registros AUDx)** — Cada canal tiene LCH/LCL (dirección del sample), LEN (cuántas palabras), PER (frecuencia), VOL (volumen). Cambiar PER cambia el tono; cambiar VOL cambia el volumen.
- **Tick** — Unidad de tiempo del reproductor: una “filas” del patrón puede durar uno o más ticks. A más ticks por fila, la música va más lenta; a menos, más rápida.
