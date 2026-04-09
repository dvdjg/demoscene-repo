# Módulos avanzados (roadmap largo plazo)

Cada bloque puede ser un **submódulo** opcional del motor, con su propio backend OCS/AGA y tests host donde tenga sentido.

## Scroll y tile engines

- Mapas ortogonales, capas múltiples, **multiscroll** (parallax por capa).
- Referencias de implementación en el repo: `effects/tiles8`, `effects/tiles16`, `effects/layers`.

Entregables del motor: API de `TileLayerConfig`, actualización de scroll, regeneración parcial de copper para fetch window si aplica.

## Split screen y copper mid-frame

- Varias “ventanas” lógicas con distinto modo o paleta.
- Requiere WAIT precisos y a veces segunda lista (`cop2`).

Documentar límites OCS (número de cambios por línea, coste en tiempo copper).

## Chunky y C2P

- Pipeline CPU → buffer chunky → `PixmapScramble_*` → blitter (`include/pixmap.h`).
- Útil para efectos tipo fade procedural o herramientas portadas.

## 3D poligonal “soft” con blitter

- Mantener como **módulo opcional** inspirado en `lib/lib3d` y efectos `flatshade` / `uvmap`.
- No bloquear el núcleo 2D del motor.

## AGA (A1200)

- Feature flags compile-time o detección en runtime (`IsAGA()` en `custom.h`).
- Backends separados para FMODE, profundidades extra y paletas ampliadas.
- La API pública no debe bifurcarse con `#ifdef`; sí los `.cpp` de backend.

## Priorización

Orden sugerido según tipo de proyecto:

1. Plataformas / run ’n’ gun: tilemap + BOBs + sprites HW.
2. Shoot’em up: scroll + muchos blits + raster bars.
3. Aventura gráfica: UI + paleta + pocos planos.

Anterior: [RHI y scheduler](09-alto-nivel-rhi-y-scheduler.md). Siguiente: [Audio, herramientas y datos](11-audio-herramientas-datos.md).
