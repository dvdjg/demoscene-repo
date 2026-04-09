# Plantilla de proyecto de juego

1. Copia `effects/engine-demo` como punto de partida (`cp -r effects/engine-demo effects/mijuego`).
2. Renombra el símbolo `EFFECT(MiJuego, ...)` y el directorio.
3. Añade `mijuego` a `effects/Makefile` → `SUBDIRS`.
4. Extiende `ae_amiga_bridge.c` (o crea `mi_juego_bridge.c`) con la lógica de copper y blitter específica.
5. Ejecuta `make -C amiga-engine host-test` antes de cada commit para validar la parte C++ compartida.

Cuando `m68k-amigaos-g++` esté en el PATH, podrás enlazar objetos `.cpp` directamente en el ejecutable Amiga y reducir el puente C.
