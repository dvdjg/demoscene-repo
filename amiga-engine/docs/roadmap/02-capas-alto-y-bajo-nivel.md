# Capas: alto y bajo nivel

## Vista en pilas

De arriba abajo (la aplicación hacia el hardware):

1. **Juego / contenido** — niveles, IA, reglas, datos (scripts, tablas, módulos de música).
2. **API pública C++** — escena retenida, entidades, componentes lógicos, recursos tipados (`std::span`, handles), bucle de actualización explícito.
3. **Motor (subsistemas)** — “managers” o servicios: gráficos abstractos, audio, entrada, tiempo, carga de recursos.
4. **RHI Amiga (render hardware interface)** — traducción de primitivas abstractas a **copper + blitter + CPU** con presupuesto por frame.
5. **HAL / núcleo chip-safe** — acceso a `Custom`, CIA, interrupciones, esperas de blitter, clasificación chip vs fast.
6. **Framework existente** — `MemAlloc`, `CopList*`, `Blitter*`, drivers de teclado, etc.
7. **Hardware** — OCS/ECS/AGA.

## Flujo de un frame (objetivo)

```text
update(dt)     →  lógica de juego, animaciones, física ligera
build_scene()  →  ordenar drawables, capas, prioridades dual-PF / sprites
record_copper()→  lista(s) copper para modo, ventana, paleta, splits
enqueue_blits()→  cola de operaciones blitter (y opcionalmente CPU blits)
flush()        →  activar copper, drenar blitter, sincronizar vblank
```

En **host**, las fases `record_copper` y `enqueue_blits` deben poder ejecutarse contra **simuladores o codificadores** sin hardware.

## Alto nivel (responsabilidades)

| Pieza | Rol |
|-------|-----|
| `Scene` / capas | Composición, orden relativo, semántica de juego (fondo, PF A/B, BOBs, UI). |
| Políticas de sprite | Misma entidad lógica puede mapearse a sprite HW, BOB, softsprite o efecto copper. |
| `GameLoop` / tiempo | Paso fijo o variable; en Amiga acoplado a PAL/NTSC y `TaskWaitVBlank` o CIA. |
| Assets / manifest | Identificadores estables, grupos de disco, conversión PNG→planar, etc. |

## Bajo nivel (responsabilidades)

| Pieza | Rol |
|-------|-----|
| HAL registros | Constantes y envoltorios type-safe sobre offsets de `Custom`. |
| Memoria | Arenas por frame, alineación, `MEMF_CHIP` vs `MEMF_FAST`. |
| Copper | WAIT/MOVE/SKIP, doble buffer de listas, límites VP/HP PAL. |
| Playfield | `BPLCON0/1/2`, DIW/DDF, dual playfield, HAM (si aplica). |
| Sprites HW | Ocho canales, attach, datos en chip. |
| Blitter | Minterms, máscaras, fill, líneas, arbitraje con CPU. |

## Frontera C / C++

- **C++**: API de juego, metadatos de escena, tests host, utilidades sin `volatile`.
- **C o C++ “sistema”**: IRQ, punteros a chip, secciones especiales, llamadas a funciones del framework que ya están en C.

Mantener esta frontera explícita evita arrastrar excepciones, RTTI o iostream al binario final si no se desea.

Anterior: [Introducción](01-introduccion-y-alcance.md). Siguiente: [Hitos y fases](03-hitos-fases.md).
