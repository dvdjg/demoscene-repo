# Tutorial: Floor

## Objetivo

Efecto de **suelo** con franjas que se mueven hacia el espectador (o que rotan), con sensación de profundidad. La imagen base es un **bitmap en escala de grises** precalculado; la animación se logra **cambiando la paleta por franja y por línea** y aplicando **scroll horizontal** vía BPLCON1 en el copper.

## Archivos clave

- `effects/floor/floor.c` — GenerateShifterValues, MakeCopperList (BPLCON1 por línea), InitStripes, ColorizeStripes, ShiftStripes.
- `data/floor.c` — Bitmap del suelo (planos).
- `data/stripes.c`, `data/stripeWidth.c`, `data/stripeLight.c` — Colores por franja, anchura por scanline, nivel de luz por scanline.

## Flujo

1. **Init**: GenerateShifterValues() rellena una tabla que luego se usa para escribir BPLCON1 (valores para PF1Px/PF2Px) según offset y scanline. InitStripes() inicializa cada “stripe” con un color base y un paso de animación. Crea dos copper lists; en cada una, **por cada scanline** hay un WAIT y un MOVE a bplcon1 (y cada 8 líneas, reseteo de colores). CopListActivate con una de las listas.
2. **Render**: ShiftColors() rota los colores de las franjas (avanza el offset). ColorizeStripes() recorre las franjas y, usando stripeLight (oscuridad por línea), escribe en las instrucciones copper el color final para cada franja en cada “bloque” de líneas. ShiftStripes() aplica el desplazamiento horizontal: para cada scanline lee el valor de shifterValues y escribe ese valor en la instrucción MOVE de bplcon1 correspondiente. Así el bitmap estático parece moverse. Se hace CopListRun(cp[active]), VBlank, se intercambia active.
3. **Kill**: desactiva DMA, borra listas.

## Técnicas y trucos

- **Bitmap estático, animación por paleta y scroll**: no se redibuja el suelo; solo se cambian los 16 colores (por franja y por “nivel de luz”) y el registro BPLCON1 por línea. Muy económico.
- **BPLCON1 por línea**: BPLCON1 controla el scroll horizontal fino (y otras cosas). Escribirlo en cada scanline desde el copper produce un desplazamiento distinto por línea; combinado con una tabla (shifterValues) se simula el avance de las franjas.
- **stripeLight**: tabla que da un factor de “oscuridad” (0–11) por scanline; las franjas se oscurecen hacia arriba o abajo para dar sensación de profundidad.
- **StripeT**: cada franja tiene step (cuándo pasar al siguiente color), orig (color base) y color (actual). El “color cycling” por franja se hace en CPU y se vierte a las instrucciones copper.
- **Doble buffer de copper**: una lista se muestra mientras la otra se rellena con los nuevos colores y valores BPLCON1; en VBlank se cambia cuál está activa.

## Conceptos para principiantes

- **BPLCON1** — Registro de control del playfield: bits PF1Px, PF2Px (scroll horizontal fino en lores). Cambiarlo por línea hace que cada línea muestre los mismos datos de bitplanes pero “desplazados” un número de píxeles.
- **Modo dual playfield** — (Si se usara) dos playfields independientes con 8 colores cada uno. En este efecto el suelo puede ser un solo playfield con muchas franjas simuladas por paleta.
- **Copper por línea** — Patrón muy común: un WAIT(cp, Y(i), …) seguido de uno o varios MOVE que configuran color, bplcon1, etc. solo para esa línea. Así se puede tener efecto distinto por scanline sin más memoria de imagen.
