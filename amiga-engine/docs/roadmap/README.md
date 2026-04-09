# Roadmap y referencia del motor (`amiga-engine`)

Este directorio agrupa el **roadmap evolutivo** y la **referencia por capas** del motor experimental. Está pensado para crecer: añade nuevas páginas numeradas o por tema y enlázalas desde aquí.

## Cómo leerlo

1. [Introducción y alcance](01-introduccion-y-alcance.md) — objetivos, no-objetivos, relación con el framework demoscene.
2. [Capas: alto y bajo nivel](02-capas-alto-y-bajo-nivel.md) — mapa conceptual y dependencias entre subsistemas.
3. [Hitos y fases](03-hitos-fases.md) — orden sugerido de trabajo y entregables.
4. **Bajo nivel**
   - [Runtime y sistema](04-bajo-nivel-runtime-y-sistema.md) — bucle, vblank, takeover, IRQ, E/S.
   - [HAL y memoria](05-bajo-nivel-hal-memoria.md) — registros, chip/fast, arenas.
   - [Copper, playfield y sprites HW](06-bajo-nivel-copper-playfield-sprites.md) — listas, modos, prioridades.
   - [Blitter](07-bajo-nivel-blitter.md) — colas, minterms, presupuesto de frame.
5. **Alto nivel**
   - [Escena retenida y políticas](08-alto-nivel-escena-y-politicas.md) — `Scene`, capas, variantes de sprite.
   - [RHI y planificación de frame](09-alto-nivel-rhi-y-scheduler.md) — interfaz de render y orden CPU/copper/blitter.
6. [Módulos avanzados](10-modulos-avanzados.md) — scroll, split, c2p, 3D opcional, AGA.
7. [Audio, herramientas y datos](11-audio-herramientas-datos.md) — pipeline de assets y formato de juego.
8. [Pruebas, calidad y CI](12-pruebas-calidad-ci.md) — unitarias, integración, regresión visual, agentes.
9. [Ampliar esta referencia](13-ampliacion-del-documento.md) — convenciones para nuevas páginas.

## Documentos relacionados (fuera de `roadmap/`)

- [Arquitectura resumida](../ARCHITECTURE.md)
- [CI y agentes](../AGENT_CI.md)
- [Contribución](../CONTRIBUTING.md)
- [Tutorial primer juego](../TUTORIAL_FIRST_GAME.md)
