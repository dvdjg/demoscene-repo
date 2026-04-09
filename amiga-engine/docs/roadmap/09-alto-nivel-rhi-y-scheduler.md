# Alto nivel: RHI y planificación de frame

## RHI (Render Hardware Interface)

Capa intermedia que **traduce** la escena ya resuelta en:

- Instrucciones copper (una o dos listas por frame).
- Cola de blitter.
- Actualizaciones de sprites HW y, si aplica, escrituras CPU a buffers chunky previos a C2P.

Debe ser **agnóstica del género** del juego pero **consciente del chipset** (OCS vs AGA) mediante backends inyectables o tablas de capacidades.

## Scheduler de frame

Orden sugerido (ajustable):

1. Preparar copper “estática” o incremental (modo, paleta base, punteros de fondo).
2. CPU: scroll, tilemap, preparación de máscaras.
3. Blitter: de atrás hacia adelante según prioridad visual.
4. Sprites HW: último toque antes o después del blitter según el truco concreto.
5. Barridos copper por línea (raster FX) si están activos.

## Paralelismo CPU / blitter

Donde sea seguro, **encadenar** trabajo CPU mientras el blitter corre en la operación anterior; el roadmap debe marcar qué pasos son candidatos y cuáles requieren barrera explícita.

## Depuración y perfiles

- Contador de blits por frame en builds `DEBUG`.
- Integración opcional con UAE traps / profiler del framework (`PROFILER` en `config.h`).

## Tests sugeridos

- En host: RHI “nulo” que solo registre orden de llamadas (mock) para validar el scheduler.
- En target (futuro): métrica de líneas de raster entre dos marcadores.

Anterior: [Escena y políticas](08-alto-nivel-escena-y-politicas.md). Siguiente: [Módulos avanzados](10-modulos-avanzados.md).
