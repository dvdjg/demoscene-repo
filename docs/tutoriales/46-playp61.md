# Tutorial: PlayP61 (Reproducción P61)

## Objetivo

Reproducir módulos en formato **P61** (Protracker 6.1 / formato relacionado) usando el **driver P61**. El efecto carga el módulo, inicializa P61 y cada frame (o en una interrupción) llama al reproductor para que actualice **Paula** y suene la música.

## Archivos clave

- `effects/playp61/playp61.c` — Init (carga módulo, P61_Init o similar), Render (P61_Music o P61_Play cada frame), Kill (P61_End, silenciar).
- Código del driver P61: lectura de patrones, efectos, escritura en registros de Paula.

## Flujo

1. **Init**: Carga el archivo del módulo (P61) desde disco o desde datos. **P61_Init**(module_ptr, chip_mem_flag) o equivalente: el driver reserva buffers en chip para samples, parsea la cabecera y deja listo el primer patrón. Configura pantalla si hay visualización.
2. **Render**: Cada frame: **P61_Music()** o **P61_Play()** — el driver avanza la posición en el patrón, aplica efectos (vibrato, portamento, etc.) y escribe en los registros **AUDxLCH, AUDxLEN, AUDxPER, AUDxVOL**. Opcional: dibujar scope o VU. TaskWaitVBlank.
3. **Kill**: **P61_End()** o similar para liberar recursos; silenciar canales (volumen 0); liberar memoria del módulo.

## Mecanismos de hardware que lo hacen posible

- **Paula**: Reproduce los **4 canales** leyendo muestras desde **chip RAM** por DMA. El driver P61 actualiza **AUDxLCH/LCL** (puntero al sample), **AUDxLEN** (longitud), **AUDxPER** (frecuencia), **AUDxVOL** (volumen). Paula sigue reproduciendo hasta que el sample termina o el driver cambia los registros (nueva nota).
- **CPU**: El driver P61 corre en la CPU: interpreta el módulo (patrones, notas, comandos de efectos), calcula periodos y escribe en Paula. Suele invocarse una vez por VBlank o cada N ciclos.
- **Chip RAM**: Los samples deben estar en chip RAM. P61 suele copiar o descomprimir los samples del módulo a buffers en chip.
- **Copper / Blitter**: Solo para la parte gráfica (si hay); el sonido es independiente.

## Técnicas y trucos

- **P61 en interrupción**: Muchos demos llaman a P61_Music desde la interrupción de vertical blank para que la música avance a 50 Hz independientemente del framerate del efecto gráfico. Así la música no se acelera si el efecto va más rápido.
- **Replay rate**: Algunos drivers permiten cambiar la frecuencia de “replay” (cuántas veces por segundo se avanza una fila). A 50 Hz es estándar en PAL.
- **Visualización**: Leer los punteros de sample o los registros de volumen para dibujar barras o formas de onda en pantalla.

## Conceptos para principiantes

- **P61** — Driver de reproducción de módulos (estilo Protracker) para Amiga. Optimizado para bajo uso de CPU y compatibilidad con muchos efectos de tracker.
- **Paula DMA de audio** — Paula lee las muestras de memoria automáticamente; la CPU no tiene que enviar cada muestra. Solo hay que decir “empieza en esta dirección, esta longitud, este periodo y volumen”.
- **Módulo** — Archivo que contiene patrones (qué nota en cada canal en cada fila), samples (waveforms) y configuración. El driver “reproduce” el módulo escribiendo en Paula según el contenido.
