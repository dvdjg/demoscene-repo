# Tutorial: UVLight (Luz UV)

## Objetivo

Efecto de **luz UV** (ultravioleta): simular que ciertas zonas o objetos “brillan” como bajo luz negra (colores neón, fluorescencia). Se suele lograr con una **paleta** donde los colores “normales” son oscuros y los que representan “fluorescencia” son muy saturados (magenta, cyan, amarillo); opcionalmente **color cycling** o parpadeo suave. Se dibuja la escena (objetos, texto) con esos colores y a veces un **fondo oscuro** (casi negro) para que el contraste recuerde a luz UV.

## Archivos clave

- `effects/uvlight/uvlight.c` — Init (bitmap, copper, paleta “UV”: fondos oscuros, acentos fluorescentes), Render (dibuja formas o texto con los índices “fluorescentes”; opcional animación de paleta).
- Posible uso de BlitterLine/BlitterFill para las formas.

## Flujo

1. **Init**: Crea bitmap y copper. Define **paleta UV**: COLOR00 = negro o muy oscuro; COLOR01–07 = tonos que simulan fluorescencia (magenta, cyan, verde ácido, amarillo). Carga LoadColors. Activa DMA.
2. **Render**: Dibuja el **fondo** con color oscuro (índice 0). Dibuja **objetos** (líneas, texto, formas) con los colores “fluorescentes” (índices 1–7). Opcional: **color cycling** en esos índices (cambiar ligeramente RGB cada frame) para que “pulsen”. TaskWaitVBlank.
3. **Kill**: Libera bitmap y copper.

## Mecanismos de hardware que lo hacen posible

- **Copper**: **LoadColors** define la paleta. Para efecto UV, los valores RGB de los colores “fluorescentes” son muy saturados (R o G o B altos, los otros bajos). La copper puede **cambiar** esos colores por línea (degradado) o el efecto puede ser estático. No hay chip especial “UV”; es puramente elección de paleta.
- **Blitter / CPU**: Dibujan las formas (líneas, rellenos, texto) en el bitmap usando los índices de color que corresponden a la “fluorescencia”. El hardware solo muestra esos índices con el RGB que LoadColors definió.
- **DMA de raster**: Muestra el bitmap; el “efecto UV” es visual (paleta) no un modo de video especial.
- **Chip RAM**: Bitmap en chip.

## Técnicas y trucos

- **Paleta**: Negro (0,0,0) o azul muy oscuro para “fondo”. Fluorescencia: (0, 0, 15) cyan, (15, 0, 15) magenta, (0, 15, 0) verde. En Amiga los valores suelen ser 0–15 por canal (4 bit). Esos colores “saltan” sobre el fondo oscuro.
- **Color cycling**: Cada frame: COLOR02 = (r, g+1, b) & 15, etc. Un ligero cambio hace que parezca que “brillan” o pulsan.
- **Doble plano**: Fondo en un playfield oscuro y “objetos UV” en otro (dual playfield) para que el fondo no tenga que redibujarse; solo se dibujan los objetos con color 0 transparente fuera de ellos.
- **Sin hardware UV** — No existe un modo “luz negra” en el Amiga; todo es elección de colores y contraste.

## Conceptos para principiantes

- **Luz UV (simulada)** — En gráficos se simula con fondo muy oscuro y colores muy saturados (cyan, magenta, amarillo) que recuerdan a la fluorescencia bajo luz negra. Es un efecto artístico de paleta.
- **Color cycling** — Cambiar los valores RGB de la paleta en el tiempo. Los píxeles no cambian; lo que cambia es “qué color muestra cada índice”. Da animación barata.
- **Saturación** — Un color “saturado” tiene un canal (R, G o B) alto y los otros bajos (o cero). En pantalla se ve muy “puro” (cyan puro, magenta puro), ideal para efecto neón/UV.
