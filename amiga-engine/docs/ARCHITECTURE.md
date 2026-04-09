# Arquitectura del motor (amiga-engine)

## Roadmap y referencia extendida

Documentación larga por capítulos (alto/bajo nivel, fases, pruebas): [docs/roadmap/README.md](roadmap/README.md).

## Capas

1. **API pública C++** (`include/amiga_engine/*.hpp`): escena retenida, colas de blitter semánticas, codificación de copper para pruebas en host, políticas de sprite.
2. **Puente Amiga en C** (`c/ae_amiga_bridge.c`): integración con `libgfx` / `copper.h` hasta que el cruce `m68k-amigaos-g++` esté disponible de forma uniforme.
3. **Tests host** (`tests/host/`): ejecutables con `g++` nativo (C++20), salida TAP.

## Flujo de frame (objetivo)

`Scene::build_blitter_frame` → `BlitterQueue` → (Amiga) rutinas de `lib/libblit`; copper list paralela vía `CopperProgramBuilder` o helpers existentes.

## Toolchain

- **Host**: `make -C amiga-engine host-test`
- **Amiga**: el efecto `effects/engine-demo` enlaza el puente C y las bibliotecas del framework existente.
