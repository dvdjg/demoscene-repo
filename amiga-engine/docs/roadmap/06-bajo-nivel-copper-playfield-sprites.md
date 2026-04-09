# Bajo nivel: copper, playfield y sprites hardware

## Copper

El copper ejecuta un microprograma sincronizado con el haz. Responsabilidades típicas del motor:

- **MOVE** a paleta, punteros de bitplanes, modos, ventanas de display, prioridades.
- **WAIT** / **SKIP** con las peculiaridades PAL (VP > 255, máscaras, etc.; ver `include/copper.h`).

Roadmap técnico:

1. **Builder semántico** (operaciones lógicas WAIT/MOVE/END) con **codificación testeable en host** (`CopperProgramBuilder`).
2. **Volcado** a `CopListT` real del framework (`NewCopList`, `CopListFinish`, `CopListActivate`).
3. **Doble buffer** de listas para modificar entre frames sin parpadeos.

## Playfield

Alineado con `include/playfield.h` y `CopSetupMode` / `CopSetupDisplayWindow` / `CopSetupBitplanes`:

- Modo lores/hires, profundidad, **dual playfield**, HAM (si el juego lo requiere).
- DIW/DDF en coordenadas coherentes con el diseño del nivel.

El descriptor de alto nivel (`PlayfieldConfig` en C++) debe mapearse 1:1 a estas llamadas sin que el juego tenga que conocer los registros.

## Sprites hardware (OCS)

- Ocho sprites simples, posicionamiento y datos en chip; **attach** para ancho extra.
- Colisión con prioridades respecto a playfields (vía `BPLCON2` y orden en copper).

Roadmap: tipo `HwSpriteChannel`, validación de límites y helpers que alimenten tanto copper como DMA sprite.

## Tests sugeridos

- **Golden streams**: comparar `encode_words()` con tablas esperadas para WAIT/MOVE/END.
- **Integración**: lista mínima que solo cambie `color0` y termine correctamente (ya cercano al efecto demo).

Anterior: [HAL y memoria](05-bajo-nivel-hal-memoria.md). Siguiente: [Blitter](07-bajo-nivel-blitter.md).
