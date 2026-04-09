# Bajo nivel: blitter

## Rol en el motor

El blitter es el coprocesador para copiar, rellenar, enmascarar y trazar líneas. El motor debe tratarlo como un **recurso compartido** con reglas claras:

- No asumir que el blitter está libre tras una llamada sin esperar (`BlitterWait` / equivalente del framework).
- Serializar operaciones si varias capas las generan en el mismo frame.

## Cola de operaciones

Patrón recomendado:

1. **Fase de registro**: `update` y `build_scene` solo **encolan** `BlitOp` (tipo, rectángulos, minterm, máscaras).
2. **Fase de flush**: traducción a llamadas de `lib/libblit` o a programación directa de registros (solo si hay ganancia medida).

## Presupuesto por frame

- Límite en **número de ops** o en **tiempo estimado** (heurístico + medición en raster en builds profile).
- **Degradación**: reducir partículas, LOD de capas, saltar blits no críticos.

## API de alto nivel (sin registros)

Funciones tipo `blit_sprite_masked`, `clear_region`, `blit_tile` deben vivir encima de la cola y ocultar minterms y shifts.

## Simulador host

Un framebuffer planar simplificado (como el actual `PlanarFramebuffer`) permite regresión sin emulador. Roadmap de ampliación:

- Multiples planos o formato chunky intermedio.
- Máscaras y minterms reducidos a casos de juego comunes.

## Tests sugeridos

- Fill y copy con oráculo byte a byte.
- Presupuesto: encolar más ops de las permitidas y comprobar truncamiento o error según política.

Anterior: [Copper y playfield](06-bajo-nivel-copper-playfield-sprites.md). Siguiente: [Escena y políticas](08-alto-nivel-escena-y-politicas.md).
