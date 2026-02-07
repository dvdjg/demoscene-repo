# Tutorial: PlayProtracker (Reproducción Protracker)

## Objetivo

Reproducir módulos en formato **Protracker** (MOD) usando un **driver Protracker**. Es el mismo concepto que PlayAHX, PlayCinter y PlayP61: cargar el módulo, inicializar el reproductor y cada frame (o en VBlank) llamar al driver para que actualice **Paula** y suene la música.

## Archivos clave

- `effects/playprotracker/playprotracker.c` — Init (carga .mod, inicializa driver), Render (llamada al reproductor cada frame), Kill (detener, liberar).
- Driver Protracker: código que lee patrones y samples y escribe en AUDx*.

## Flujo

1. **Init**: Carga el archivo .mod (o buffer con el módulo). Inicializa el driver: configura punteros a patrones y samples, copia samples a **chip RAM** si es necesario, pone velocidad/BPM. Configura pantalla si hay visualización.
2. **Render**: Cada frame: **Protracker_Play()** o **pt_play()** — el driver avanza la posición (patrón, fila), aplica efectos (0xy, 1xy, Exy, etc.) y escribe en **Paula** (AUDxLCH, AUDxLEN, AUDxPER, AUDxVOL) para cada canal que tenga nota. TaskWaitVBlank.
3. **Kill**: Silencia canales (volumen 0), libera memoria del módulo y de pantalla.

## Mecanismos de hardware que lo hacen posible

- **Paula**: **4 canales** de audio en DMA. El driver Protracker actualiza los registros **AUDxLCH/LCL** (puntero al sample en chip RAM), **AUDxLEN** (longitud en palabras), **AUDxPER** (periodo = frecuencia), **AUDxVOL** (volumen 0–64). Paula reproduce sin intervención de la CPU una vez configurado.
- **CPU**: El driver interpreta el formato MOD (patrones de 4 canales, notas con periodo y número de sample, efectos). Calcula el periodo a partir de la nota (tabla de periodos Protracker), aplica efectos (slide, vibrato, arpeggio, etc.) y escribe en Paula. Se ejecuta una vez por frame o por tick.
- **Chip RAM**: Samples en chip RAM para DMA de Paula.
- **Copper / Blitter**: Solo para gráficos si los hay; el audio no los usa.

## Técnicas y trucos

- **Formato MOD**: 31 samples (o 15 en versiones antiguas), patrones de 64 filas, 4 canales. Cada “celda” tiene nota (periodo), número de sample y efecto (comando + parámetro). El driver debe implementar los efectos que use el módulo (E01, E11, etc.).
- **Tabla de periodos**: Protracker usa una tabla fija de periodos para cada nota (Amiga). La nota C-4 suele tener un periodo concreto; el driver busca en la tabla y escribe ese valor en AUDxPER.
- **Visualización**: Igual que en otros reproductores: leer niveles o muestras para dibujar scope o barras en pantalla.

## Conceptos para principiantes

- **Protracker / MOD** — Formato de música tracker muy usado en Amiga. Archivo .mod con cabecera (nombres de samples, órden de patrones), samples en raw, y patrones (secuencia de notas y efectos por canal).
- **Efectos de tracker** — Comandos en cada celda: por ejemplo 0xy = arpeggio, 1xy = slide up, 2xy = slide down, E1x = fine slide up. El driver tiene que implementar cada efecto para que suene correctamente.
- **Paula** — Chip de sonido: 4 canales, 8 bits, DMA. La CPU solo actualiza registros; Paula hace la reproducción en paralelo.
