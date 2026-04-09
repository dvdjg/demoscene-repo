# Audio, herramientas y datos

## Audio

El ecosistema demoscene ya incluye rutas para PT/P61/AHX y similares (`lib/`, `include/`). El motor debe ofrecer:

- **Interfaz unificada** (`play_music`, `stop`, volumen, prioridad SFX).
- Separación entre **driver** (chip Paula, buffers) y **lógica de juego** (cuándo disparar un sonido).

Roadmap: implementación concreta por encima de `NullAudioBus` en `audio.hpp`.

## Pipeline de assets

- Conversión **PNG → planar** con paletas y máscaras (herramientas existentes en `tools/`).
- IDs estables en manifest (`assets.hpp` → evolucionar a formato serializado).
- Versionado de paquetes para no romper saves o niveles.

## Datos de juego y niveles

- Formato abierto (binario con cabecera, JSON comprimido, etc.) inspirado en flujos tipo Scorpion pero sin cerrar el esquema.

Temas a documentar en páginas futuras:

- Entidades por nivel (spawn, triggers).
- Referencias cruzadas a gráficos y sonidos por id.
- Hot reload solo en host (opcional).

## Editor

Fuera del núcleo del motor a corto plazo; el roadmap contempla **exportadores** desde Tiled u otras fuentes si se adoptan (`tools/tmxconv` ya existe en el repo).

Anterior: [Módulos avanzados](10-modulos-avanzados.md). Siguiente: [Pruebas y CI](12-pruebas-calidad-ci.md).
