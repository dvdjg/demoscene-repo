# Tutorial: Weave (Tejido)

## Objetivo

Efecto **weave** (tejido): patrón que parece **tela** o **cesta** entretejada: bandas horizontales y verticales (o curvas) que se **cruzan** y en los cruces se simula que una pasa “por encima” o “por debajo” de la otra (alternando). Se dibuja con **líneas** o **bandas rellenas**; el “orden” en cada cruce puede determinarse por una **tabla** (par/impar de fila y columna) o por **offset** (desplazamiento seno). El **blitter** rellena las bandas y dibuja los bordes; la **CPU** calcula las posiciones y qué banda va encima en cada cruce.

## Archivos clave

- `effects/weave/weave.c` — Init (bitmap, copper), Render (dibuja bandas horizontales y verticales; en cada intersección, dibuja un pequeño “corte” o cambia el color para que una banda parezca pasar por encima).
- Posible tabla par/impar: (row + col) % 2 decide si en ese cruce la horizontal va encima o la vertical.

## Flujo

1. **Init**: Crea bitmap y copper. Define parámetros: espaciado entre bandas (cada N píxeles), grosor de cada banda, paleta (dos tonos para “arriba” y “abajo” o un solo color con “cortes”). LoadColors. Activa DMA.
2. **Render**: **Bandas horizontales**: para cada fila y = row * spacing (row = 0,1,2,…), dibuja una banda de grosor G (líneas desde (0, y) hasta (width, y) o relleno entre y y y+G). **Bandas verticales**: para cada columna x = col * spacing, dibuja banda vertical. En cada **intersección** (x,y): según (row+col)%2 (o una función), **borrar o dibujar** un pequeño segmento de una de las dos bandas para que la otra “pase por encima” (o se dibuja primero la que va detrás y luego la que va delante en ese cruce). Se puede usar **BlitterFill** para las bandas y **BlitterCopy** con máscara o **CpuRect** para los “cortes”. TaskWaitVBlank.
3. **Kill**: Libera bitmap y copper.

## Mecanismos de hardware que lo hacen posible

- **Blitter**: **BlitterFill** para rellenar las bandas (rectángulos largos horizontales y verticales). En los cruces, hacer un **BlitterCopy** de “recorte”: copiar desde un buffer que tiene un hueco (la banda de “abajo” con un agujero donde pasa la de “arriba”), o dibujar las bandas en orden y en cada cruce **rellenar con el color de fondo** el trozo de la banda que debe quedar detrás (así la otra banda ya dibujada “tapa”). El blitter hace los rellenos muy rápido.
- **CPU**: Calcula **posiciones** de las bandas (y = row*spacing + offset si hay onda). Decide **orden en cada cruce** (row+col)%2. Llama al blitter con las coordenadas correctas para cada banda y cada “corte”. Si no se usa blitter para los cortes, la CPU puede escribir los píxeles del hueco.
- **Copper**: bplpt y LoadColors. Opcional: degradado por línea para dar sensación de relieve (la banda “arriba” más clara, la “abajo” más oscura).
- **DMA de raster**: Muestra el bitmap con el patrón de tejido.
- **Chip RAM**: Bitmap en chip.

## Técnicas y trucos

- **Orden de dibujo**: Dibujar primero todas las bandas que van “atrás” (por ejemplo las que en cruces pares van abajo) y luego las que van “delante”. En cada cruce, la que va delante debe tener un “corte” donde pasa la otra, o se dibuja la de atrás con huecos. Más simple: dibujar todas las horizontales; luego dibujar las verticales, pero en cada cruce **no dibujar** un pequeño segmento de la vertical si en ese cruce la horizontal va encima (ya está dibujada). Así la horizontal “tapa” la vertical.
- **Offset ondulado**: y = row * spacing + amplitude * sin(frame + row). Las bandas ondulan; el patrón de “arriba/abajo” sigue siendo (row+col)%2 en la cuadrícula lógica.
- **Dos colores**: Color A para “por encima”, color B para “por debajo”. En cada banda se usan ambos: en los tramos donde va encima, color A; donde va debajo (en el cruce), no se dibuja o color B. Da efecto 3D de cesta.
- **Máscara de cruce**: Precalcular un pequeño sprite o máscara “agujero” para el cruce; BlitterCopy con esa máscara para “recortar” la banda que va detrás.

## Conceptos para principiantes

- **Weave (tejido)** — Patrón de bandas que se cruzan; en cada cruce una pasa “por encima” de la otra de forma alternada (como una tela o cesta). Se simula dibujando en orden y haciendo “cortes” donde la banda de detrás no se dibuja.
- **Orden de pintado** — Quién se dibuja después tapa a quien se dibujó antes. Para que la banda A pase “por encima” en un cruce, se dibuja la banda B primero y luego la A (o se borra un trozo de B donde cruza A).
- **Par/impar (row+col)%2** — Regla simple para alternar: en cruces (0,0), (1,1), (2,2)… una opción; en (0,1), (1,0)… la otra. Da patrón de tablero en los cruces.
