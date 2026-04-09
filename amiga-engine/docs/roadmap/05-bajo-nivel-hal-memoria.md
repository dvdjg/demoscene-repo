# Bajo nivel: HAL y memoria

## HAL de registros

Objetivo: sustituir números mágicos sueltos por **tipos y constantes** (offsets alineados con `struct Custom`), de forma que:

- El codificador copper y el RHI compartan el mismo vocabulario.
- Los tests host puedan validar pares `(reg, valor)` sin incluir cabeceras Amiga completas.

En el código actual del motor hay un primer paso en `hal_custom_regs.hpp`; debe extenderse con el mapa que use realmente el juego (DMA, DIWSTRT, BPLxPTH, etc.).

## Chip vs fast

- **Chip**: bitplanes, sprites, audio, copper lists, algunas estructuras que el DMA lee.
- **Fast**: lógica, tablas grandes, descompresión, pathfinding — siempre que no las lea el custom chip.

El roadmap de API pública debe exponer **dominio de memoria** en la creación de recursos (`MemoryDomain` o fábricas explícitas).

## Arenas y temporales

Patrón recomendado:

- **Arena por frame** para listas copper doble-buffered y comandos volátiles.
- **Pools** para partículas o BOBs si el patrón de vida es homogéneo.

En host, `BumpArena` con `std::vector<std::byte>` sirve como referencia; en destino, respaldar con `MemAlloc(MEMF_CHIP|MEMF_CLEAR)` o regiones preasignadas en Init.

## Alineación y DMA

Documentar requisitos de alineación de bitplanes y de las estructuras que consumen el blitter. Referencia: HRM y comentarios en `lib/libgfx`.

## Tests sugeridos

- Comprobar que un recurso “chip” ficticio no se coloca en fast (cuando haya metadatos de runtime).
- Estresar la arena: agotamiento → degradación controlada o assert en debug.

Anterior: [Runtime y sistema](04-bajo-nivel-runtime-y-sistema.md). Siguiente: [Copper, playfield y sprites](06-bajo-nivel-copper-playfield-sprites.md).
