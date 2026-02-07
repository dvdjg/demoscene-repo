# Tutorial: Highway

## Objetivo

Efecto de **carretera** con varias “zonas” en pantalla: cielo/ciudad arriba, carriles con coches en el medio (bitmaps que se desplazan), y ciudad abajo. Se usan **varias secciones en la copper** que activan/desactivan bitplanes y cargan paletas distintas por franja; los coches pueden ser **sprites** o blits; el movimiento es scroll horizontal de los bitmaps de carriles.

## Archivos clave

- `effects/highway/highway.c` — MakeCopperList (zonas con CopWait, dmacon, LoadColors, CopSetupBitplanes, bpldat trick), Init (lanes, carry, sprites), Render (scroll de carriles, actualización de sprites).
- `data/city-top-2.c`, `data/city-bottom-2.c`, `data/lane.c`, `data/car-left-2.c`, `data/car-right-2.c`, `data/sprite.c`.

## Flujo

1. **Init**: crea bitmaps para los dos carriles (doble buffer), bitmap “carry” para copiar coches. Carga gráficos de ciudad arriba/abajo y carriles. Construye **una** copper list que define **zonas verticales**:  
   - Arriba: WAIT hasta LANEL_Y-2, LoadColors (ciudad), DMA on, bitplanes = city_top.  
   - Zona carril izquierdo: WAIT, LoadColors (coches izq), bitplanes = lanes[active], modulo 8.  
   - Zona “entre” carriles: WAIT por cada línea y MOVE a bpldat (truco para que sprites sigan visibles con bitplanes off).  
   - Zona carril derecho: LoadColors (coches der), bitplanes = lanes.  
   - Abajo: LoadColors (ciudad abajo), bitplanes = city_bottom.  
   Además CopSetupSprites y actualización de posiciones de sprites. Activa la lista.
2. **Render**: scroll horizontal de los carriles (desplazar datos o cambiar bplcon1/ddfstrt). Actualiza posiciones de coches (sprites o blits desde carry). CopInsSetSprite para cada sprite; SpriteUpdatePos. CopListRun (o la lista se actualiza cada frame con los bplptr de lanes[active]). Alterna active para doble buffer de carriles.
3. **Kill**: libera bitmaps y copper.

## Técnicas y trucos

- **Varias “pantallas” en una**: la copper cambia bitplanes y paleta por franja vertical. Arriba = un bitmap (ciudad), centro = carriles (scroll), abajo = otro bitmap (ciudad). Sin dual playfield se hace activando solo los bitplanes en cada franja (dmacon) y cargando los punteros y colores adecuados.
- **dmacon (DMA control)**: quitar DMAF_RASTER en una franja “apaga” el refresco de bitplanes; así no se ven datos de otra zona. El “truco” de bpldat mantiene algo de estado para que los sprites sigan visibles (documentado en comentarios).
- **Sprites para coches**: hasta 8 sprites; cada uno tiene posición (SpriteUpdatePos) y datos de imagen. Se cargan en la copper con CopInsSetSprite. Los colores de sprites usan registros 17–31.
- **Scroll horizontal de carriles**: mover el contenido del bitmap de carriles cada frame (blitter) o usar BPLCON1 por línea para desplazar; los punteros bplptr apuntan a los buffers de lanes que se van rellenando con el frame actual.

## Conceptos para principiantes

- **dmacon** — Registro que habilita/deshabilita DMA de raster, blitter, copper, etc. Quitar DMAF_RASTER deja de mostrar bitplanes en esa zona.
- **CopSetupSprites** — Añade a la copper los MOVE para los punteros de sprites (SPR0PT, …). Luego CopInsSetSprite actualiza la dirección del dato de cada sprite; SpriteUpdatePos actualiza VSTART/HSTART/VSTOP en ese dato.
- **Múltiples paletas por zona** — LoadColors en distintos WAIT permite que cada franja tenga su propia paleta (ciudad vs carriles vs coches).
