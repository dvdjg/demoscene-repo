# Tutorial: primer juego con amiga-engine (borrador)

## 1. Bucle de efecto

Los juegos se integran como un `EffectT` (ver `include/effect.h`): `Init` reserva recursos, `Render` avanza un frame, `Kill` libera.

## 2. Usar el puente actual

`AeEngine_Init` configura una copper list mínima con `CopSetupMode` y ventana de display. Amplía `ae_amiga_bridge.c` para:

- reservar `BitmapT` con `NewBitmap` / chip RAM;
- fijar bitplanes en la copper list (`CopSetupBitplanes`);
- encolar blits con las funciones de `lib/libblit`.

## 3. Escena C++ en host

En el PC, construye un `amiga::engine::Scene`, añade entidades con `BobSpritePolicy` y valida la cola con `BlitterQueue` + `PlanarFramebuffer` antes de portar a hardware.

## 4. Siguientes pasos

- Sustituir los stubs de `ISpritePolicy::enqueue_draw` por datos reales de máscara y posición.
- Añadir lectura de entrada con `system/drivers/keyboard.c` y similares.
