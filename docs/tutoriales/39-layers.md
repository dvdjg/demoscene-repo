# Tutorial: Layers (Credits)

## Objetivo

Efecto de **capas**: varios planos de bits (o dos playfields) mostrados a la vez con **prioridad** (uno delante de otro). En el código se registra como “Credits”; muestra contenido que puede ser texto o gráficos en varias “capas” usando los bitplanes y la prioridad del hardware.

## Archivos clave

- `effects/layers/layers.c` — Init (varios bitmaps o un bitmap con varios “planos” usados como capas, copper con prioridad correcta), Render (actualiza contenido de capas o solo VBlank).
- La prioridad en Amiga se controla con **BPLCON2** (bits PF1P, PF2P) o con el orden de los planos.

## Flujo

1. **Init**: Crea bitmap(s). Si son dos playfields (dual playfield), configura MODE_DUALPF y asigna planos 0–2 a un playfield y 3–5 al otro; cada uno tiene su paleta (COLOR00–07 y COLOR08–15). Si es un solo playfield con “capas” lógicas, puede ser un bitmap de 5–6 planos donde los planos altos se dibujan encima (prioridad por plano). Configura copper (bplpt para todos los planos, LoadColors para ambas mitades de paleta si dual). Activa DMA.
2. **Render**: Actualiza el contenido de una o más capas (por ejemplo scroll en una, texto estático en otra) con el blitter. La “prioridad” ya está fija en la configuración del playfield; lo que se dibuje en los planos “altos” tapa lo de los “bajos”. TaskWaitVBlank.
3. **Kill**: Libera bitmaps y copper.

## Mecanismos de hardware que lo hacen posible

- **Denise (prioridad)**: El hardware decide **qué plano “gana”** cuando varios planos tienen píxel no cero en la misma posición. Eso se controla con **BPLCON2** (y en OCS con el orden de los planos: el de número más alto suele ir delante). Así se obtienen “capas” sin hacer nada en software: lo que está en el plano 4 tapa lo del plano 0–3.
- **Dual playfield**: Con **MODE_DUALPF** hay dos playfields independientes (cada uno con 3 planos = 8 colores). Cada uno tiene su paleta (COLOR00–07 y COLOR08–015). La prioridad entre los dos se controla con BPLCON2. Muy útil para “fondo” + “personajes” o “fondo” + “texto”.
- **Blitter**: Dibuja en cada “capa” (cada conjunto de planos). BitmapCopy o BlitterCopyMasked a las posiciones correctas; el hardware de video ya se encarga de combinar según prioridad.
- **Copper**: Configura bplpt para todos los planos y carga las dos paletas (o una si es un solo playfield). Puede cambiar prioridad por línea (BPLCON2 en la copper) para efectos especiales.
- **DMA de raster**: Lee todos los planos y los combina según prioridad para formar la imagen final.
- **Chip RAM**: Todos los planos en chip.

## Técnicas y trucos

- **Dos playfields**: Si se usa dual playfield, un playfield puede ser el “fondo” (scroll, paisaje) y el otro los “sprites” o texto. Cada uno 8 colores; 16 colores en total en pantalla.
- **Prioridad por plano**: En un solo playfield, los planos más altos (4, 5) se dibujan “encima” de los bajos. Se puede usar el plano 0–2 para fondo y el 3–5 para overlays.
- **BPLCON2**: Registro que controla prioridad entre playfield 1 y 2 y con sprites. Ver Amiga HRM para los bits exactos.

## Conceptos para principiantes

- **Prioridad (video)** — Orden en que se “mezclan” los planos cuando hay más de un píxel en la misma posición. El plano con prioridad mayor se ve encima.
- **Dual playfield** — Modo en el que la pantalla se divide en dos “capas” independientes de 3 planos (8 colores) cada una. Muy usado en juegos (fondo + personajes).
- **BPLCON2** — Registro de control que define la prioridad entre los dos playfields y los sprites. Se puede escribir desde la copper para cambiar prioridad por línea.
