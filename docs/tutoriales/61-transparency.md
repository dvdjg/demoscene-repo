# Tutorial: Transparency (Transparencia)

## Objetivo

Demostrar **transparencia** en Amiga: un color (normalmente **COLOR00** o el índice 0) se trata como **transparente**, de modo que en esas posiciones se ve lo que hay “debajo” (otro playfield, otro plano o el fondo). Se consigue con **prioridad** y con **registro BPLCON2** (y en ECS/AGA con **modos de mezcla**). En OCS lo más común es **dual playfield**: un playfield “detrás” y otro “delante”, y en el delantero el color 0 no se dibuja (o se deja ver el de atrás). También se puede simular con **blitter** y minterms (combinar dos imágenes con AND/OR según máscara).

## Archivos clave

- `effects/transparency/transparency.c` — Init (dos playfields o dos conjuntos de planos, copper con BPLCON2 y prioridad), Render (dibuja el “fondo” en un playfield y el “objeto con transparencia” en el otro; el color 0 del objeto deja ver el fondo).
- Uso de BPLCON2 para prioridad PF1/PF2.

## Flujo

1. **Init**: Configura **dual playfield** (MODE_DUALPF): planos 0–2 = playfield 1 (fondo), planos 3–5 = playfield 2 (delante). Carga dos paletas (COLOR00–07 para PF1, COLOR08–15 para PF2). En el playfield 2, el **color 0** (índice 0) se considera transparente: donde el bitmap del PF2 tiene 0, se muestra el PF1. BPLCON2: prioridad para que PF2 esté delante. Crea bitmaps para ambos playfields. Activa DMA.
2. **Render**: Dibuja el **fondo** en los planos 0–2 (BlitterFill o imagen). Dibuja el **objeto** (sprite, texto, forma) en los planos 3–5; donde no hay objeto se deja **0** (transparente). Así en esas zonas se ve el fondo. TaskWaitVBlank.
3. **Kill**: Libera bitmaps y copper.

## Mecanismos de hardware que lo hacen posible

- **Denise (dual playfield y transparencia)** — En **dual playfield**, los dos playfields se combinan. Donde el playfield delantero tiene **color 0**, el hardware **no dibuja** ese píxel y se muestra el playfield de atrás. Es transparencia “real” en hardware: sin blitter adicional. BPLCON2 controla qué playfield va delante y la prioridad con sprites.
- **Copper**: Configura **BPLCON0** (MODE_DUALPF), **bplpt** para los 6 planos (3+3), **LoadColors** para las dos mitades de paleta. Puede cambiar BPLCON2 por línea si se quiere prioridad variable.
- **Blitter**: Dibuja el contenido de cada playfield (fondo en PF1, objeto en PF2). Donde el objeto tiene 0, no hace falta dibujar; el hardware ya muestra el fondo.
- **DMA de raster**: Lee ambos playfields y combina según prioridad y transparencia.
- **Chip RAM**: Ambos playfields en chip.

## Técnicas y trucos

- **Color 0 transparente**: En dual playfield, el índice 0 del playfield delantero es siempre transparente. Por tanto hay que usar solo los colores 1–7 para el objeto en ese playfield (8 colores útiles por playfield, 0 = ver-through).
- **Sprites con transparencia**: Los sprites también tienen color 0 transparente. Se pueden poner encima de los playfields con la prioridad correcta (BPLCON2).
- **Simulación sin dual playfield**: Con un solo playfield se puede “simular” transparencia con el blitter: dibujar primero la capa de atrás, luego la del frente usando una **máscara** (solo escribir donde la máscara es 1). Así donde la máscara es 0 se queda el fondo. Minterm: dest = (back AND NOT mask) OR (front AND mask).

## Conceptos para principiantes

- **Dual playfield** — Modo en el que hay dos “capas” independientes de 3 planos (8 colores) cada una. Se combinan en pantalla; una puede estar delante de la otra. El color 0 del playfield delantero es transparente.
- **BPLCON2** — Registro que controla la prioridad entre playfield 1, playfield 2 y sprites. Define qué va “encima” cuando hay solapamiento.
- **Transparencia por hardware** — El chip de video no dibuja el color 0 del playfield delantero y deja ver el de atrás. No hace falta mezclar píxeles en software.
