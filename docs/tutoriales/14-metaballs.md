# Tutorial: Metaballs

## Objetivo

**Metaballs** (blobs) que se mueven por la pantalla y se “fusionan” donde se solapan. Cada metaball es un **sprite o gráfico precalculado** (un círculo o mancha suave) que se dibuja en el bitmap con el blitter (BitmapCopy o BlitterOr); la posición se actualiza cada frame y se hace doble buffer.

## Archivos clave

- `effects/metaballs/metaballs.c` — SetInitialPositions, Init (screen[2], carry, fondos), ClearMetaballs, Render (actualiza posiciones, limpia zonas, copia cada “metaball” desde carry al screen activo).
- `data/metaball.c`, `data/metaball-bg-*.c` — Gráficos de cada blob y fondos.
- `data/gen-metaball.py` — Genera los datos del blob (gradiente circular o similar).

## Flujo

1. **Init**: dos bitmaps de pantalla (doble buffer), bitmap “carry” con la imagen de un metaball (una “gota”). Copia fondos (bgLeft, bgRight) en ambos. Inicializa posiciones (pos[0][], pos[1][]) para 3 blobs. SetupPlayfield, LoadColors, copper con CopSetupBitplanes, activa DMA.
2. **Render**: actualiza posiciones de los 3 metaballs (pos[active][j].x/y en función del tiempo). **ClearMetaballs**: para cada blob, define un Area2D donde estaba el blob anterior y borra esa zona (BitmapClearArea o BlitterClear) para no dejar rastro. Luego, para cada blob, **BitmapCopy(screen, pos.x, pos.y, carry)** (o BlitterOr si se quiere que se “sumen” visualmente). Intercambia active, actualiza bplptr en copper, TaskWaitVBlank.
3. **Kill**: libera carry, listas y bitmaps.

## Técnicas y trucos

- **Metaball como sprite blit**: no se calcula la función “metaball” en tiempo real; cada blob es un bitmap pequeño (gradiente circular o elipse) que se copia en la posición actual. La “fusión” se simula dibujando con OR (o con suma si se usara modo especial); en el caso más simple es solo copia.
- **Limpieza por zona**: en lugar de borrar toda la pantalla, se borra solo el rectángulo que cubría cada blob en el frame anterior. Así se ahorra tiempo y se evita parpadeo del fondo.
- **Doble buffer**: un buffer se muestra mientras el otro se dibuja; al final del frame se intercambian los punteros en la copper.
- **Posiciones animadas**: pos[][j].x/y se actualizan con seno/coseno o con velocidad constante para que los blobs se muevan; el “rebote” se puede hacer invirtiendo la velocidad en los bordes.
- **Carry bitmap**: un solo bitmap con la forma del blob; se copia varias veces en distintas posiciones. Tamaño fijo (SIZE+16, SIZE) para alineación con el blitter.

## Conceptos para principiantes

- **BitmapCopy(dst, x, y, src)** — Copia el bitmap src en la posición (x,y) del dst. El blitter hace el trabajo; hay que WaitBlitter si luego se usa el mismo recurso.
- **BitmapClearArea** — Borra solo un rectángulo (Area2D); más rápido que borrar todo el bitmap cuando solo cambian unas zonas.
- **Doble buffer** — Dos bitmaps; se dibuja en uno mientras el otro se muestra. En VBlank se cambia cuál está conectado a la copper (CopInsSet32(bplptr, screen[active]->planes)).
