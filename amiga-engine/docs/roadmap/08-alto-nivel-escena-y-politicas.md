# Alto nivel: escena retenida y políticas

## Escena (`Scene`)

Responsabilidades:

- Contener **entidades dibujables** con identidad estable (`EntityId`).
- Asignar **capas** con semántica de juego: fondo, playfield A/B, BOBs, UI, efectos solo-copper.
- Ordenar el trabajo de render respetando prioridades hardware (dual-PF + sprites + blitter).

`update_fixed_step(dt)` concentra lógica de animación y IA de alto nivel; **no** debe programar el blitter directamente.

## Políticas de sprite (`ISpritePolicy`)

Un mismo “sprite lógico” puede implementarse de varias formas:

| Política | Uso típico |
|----------|------------|
| Hardware | Objetos pocos, 16 px de ancho, prioridad alta. |
| BOB | Personajes con máscara, muchos frames. |
| Soft + copper | Barras de color, parallax por líneas, splits. |

El juego elige (o datos eligen) la política; el motor **encola** el trabajo adecuado en blitter/copper/sprite registers.

## Cámara y ventana lógica

Roadmap: tipo `Camera2` que mapee coordenadas mundo → pantalla y recorte contra el playfield. En scroll amplio, acoplado a tilemap y a límites DIW.

## Composición vs herencia

Preferir **componentes ligeros** o structs asociados por id antes que jerarquías profundas de clases; las políticas ya encapsulan variación por tipo de render.

## Tests sugeridos

- Orden de capas: entidades en `PlayfieldB` antes que `Bobs` en la cola resultante.
- Sustitución de política: misma entidad con `BobSpritePolicy` vs `SoftSpritePolicy` produce distintas secuencias encoladas (mock).

Anterior: [Blitter](07-bajo-nivel-blitter.md). Siguiente: [RHI y scheduler](09-alto-nivel-rhi-y-scheduler.md).
