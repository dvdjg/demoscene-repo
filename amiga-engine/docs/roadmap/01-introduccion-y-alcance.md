# Introducción y alcance

## Propósito

`amiga-engine` es una capa orientada a **videojuegos 2D (y técnicas afines)** sobre Amiga **OCS/ECS** (A500) con extensión opcional **AGA** (A1200). Combina:

- Una **API C++ moderna** (tipos fuertes, composición, pruebas en host) en la superficie de juego.
- Un **núcleo cercano al hardware** que reutiliza o se apoya en el framework existente del repositorio (`system/`, `lib/libgfx`, `lib/libblit`, etc.).
- Un **puente C** en destino cuando el toolchain cruzado no incluye C++ o se prefiere enlazado mínimo.

## Objetivos

- Reducir la fricción para autores de juegos frente a programar directamente copper/blitter a mano en cada título.
- Mantener la posibilidad de **bajar al metal** cuando una escena lo exija (demos, rutas especiales).
- Ofrecer **tests deterministas en host** para lógica de listas copper, colas de blitter y grafos de escena.
- Documentar decisiones de diseño y un **roadmap por fases** ampliable.

## No objetivos (por ahora)

- Sustituir por completo el sistema de efectos del framework ni imponer un único estilo de demo.
- Garantizar paridad 1:1 con motores cerrados tipo Scorpion o con ACE (son referencias conceptuales).
- Ocultar el 100 % de los punteros y del código ensamblador en rutas críticas; la meta es **cero punteros crudos en la API de juego**, no en todo el árbol interno.

## Referencias externas al código del repo

| Área | Rutas típicas |
|------|----------------|
| Arranque / AmigaOS | `system/loader.c`, `system/amigaos.c` |
| Copper / bitmaps | `lib/libgfx/`, `include/copper.h`, `include/playfield.h` |
| Blitter | `lib/libblit/`, `include/blitter.h` |
| 2D / 3D matemático | `lib/lib2d/`, `lib/lib3d/` |
| Efectos de laboratorio | `effects/*` |

## Convenciones del documento

- **Alto nivel**: conceptos que un juego usa a diario (escena, capas, recursos).
- **Bajo nivel**: sincronización con el chip, formatos de memoria, registros, listas copper, colas de DMA/blitter.

Siguiente: [Capas: alto y bajo nivel](02-capas-alto-y-bajo-nivel.md).
