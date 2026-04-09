# Pruebas, calidad y CI

## Pirámide de pruebas

| Nivel | Qué valida | Dónde |
|-------|------------|--------|
| Unitarias puras | Codificadores, matemáticas, colas | Host `g++` |
| Layout / formato | Interleave, tamaños de máscara | Host |
| Integración ligera | Secuencia de frame mock | Host |
| Target smoke | N frames sin crash | fs-uae / hardware |
| Visual / golden | Hash o diff de framebuffer | Emulador (futuro) |
| Rendimiento | Raster lines, #blits | Perfil o contadores |

## Salida para agentes e IA

Los fallos deben ser **binarios** y **legibles por máquina**:

- **TAP** desde `tests-host-runner` (ver `AGENT_CI.md`).
- En el futuro: JUnit XML o JSON Lines desde un harness mínimo en Amiga.

## CI

- Job rápido en Ubuntu: `make -C amiga-engine host-test` (sin toolchain cruzada).
- Job completo en contenedor demoscene: `make` completo del repositorio.

## Determinismo

- Semillas fijas en tests que usen RNG.
- Desactivar variaciones no deterministas (audio async, timers del sistema en host).

## Golden files

- Versionar junto al repo arrays de palabras copper esperados y, si se adopta, PNG de referencia pequeños.

Anterior: [Audio y datos](11-audio-herramientas-datos.md). Siguiente: [Ampliación del documento](13-ampliacion-del-documento.md).
