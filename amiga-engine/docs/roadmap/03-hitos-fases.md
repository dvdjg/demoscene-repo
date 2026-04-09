# Hitos y fases

Orden recomendado para ir de un prototipo creíble a un motor utilizable en producción indie/demoscene. Cada hito puede desglosarse en issues o subtareas en páginas futuras.

## Fase A — Cimientos reproducibles

- Toolchain documentada (gcc/g++ cruzados, perfiles debug/release).
- `make -C amiga-engine host-test` en verde en CI.
- Efecto de referencia en hardware (`effects/engine-demo` o sucesor) que demuestre copper + frame estable.

**Criterio de salida:** cualquier colaborador puede compilar y ejecutar tests host sin Amiga; el efecto referencia corre en emulador.

## Fase B — HAL y memoria

- Registros `Custom` y utilidades de espera (blitter, beam opcional).
- Políticas de allocator chip (bump por frame, límites).
- Tests: secuencias copper codificadas vs oráculos en host.

**Criterio de salida:** cambios en el codificador copper detectados por tests sin ejecutar el juego.

## Fase C — Recursos gráficos mínimos

- Descriptor de playfield (modo, profundidad, ventana, dual-PF).
- Integración con `BitmapT` / bitplanes (interleaved, punteros chip).
- Paleta y transiciones básicas.
- Sprites hardware: API mínima (posición + datos).

**Criterio de salida:** una escena estática con bitmap + paleta + opcional sprite HW.

## Fase D — Blitter como cola

- Operaciones copy/fill/masked con presupuesto por frame.
- API de alto nivel (`blit_tile`, `clear_region`, …) sin exponer registros.
- Simulador host ampliado (más planos o chunky según prioridad).

**Criterio de salida:** test host que valide una escena de blits contra un buffer de referencia.

## Fase E — Escena retenida

- `Scene` con capas y orden establecido.
- Políticas de sprite conectadas a la cola real (no solo stubs).
- Un “estrés” controlado: dual-PF + varios BOBs + barras copper (objetivo de referencia visual).

**Criterio de salida:** demo reproducible y documentada en tutorial.

## Fase F — Módulos avanzados (elegibles)

- Tilemap / scroll multifrecuencia.
- Split screen y copper mid-frame.
- C2P / `Pixmap` path.
- Opcional 3D poligonal vía blitter (módulo aparte).
- AGA: bifurcación de backend sin contaminar la API pública.

## Fase G — Audio, datos, herramientas

- Bus de audio unificado (módulo, SFX).
- Pipeline de assets y formato de nivel/entidades versionable.

## Fase H — Calidad y automatización

- Integración target (N frames sin crash, salida serial/UAE).
- Regresión visual opcional (hash/diff).
- Presupuestos de raster o conteo de blits.

---

Las fases **A–E** son el núcleo del roadmap; **F–H** se priorizan según género de juego (plataformas vs scroll vs 3D ligero).

Anterior: [Capas](02-capas-alto-y-bajo-nivel.md). Siguiente: [Runtime y sistema](04-bajo-nivel-runtime-y-sistema.md).
