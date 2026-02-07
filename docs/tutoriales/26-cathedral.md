# Tutorial: Cathedral

## Objetivo

Efecto tipo **“catedral”**: sensación de profundidad o de rayos de luz (como un techo abovedado o un túnel). Se suele lograr con **líneas** que convergen hacia un punto (perspectiva) o con un bitmap precalculado cuyos **colores se animan por línea** con el copper.

## Archivos clave

- `effects/cathedral/cathedral.c` — Init (bitmap, copper), Render (dibuja líneas o actualiza paleta/copper por línea).
- Puede usar datos estáticos (un bitmap de “catedral”) y solo animar la paleta, o dibujar líneas con BlitterLine o CpuLine.

## Flujo

1. **Init**: Crea bitmap(s). Puede dibujar una vez un conjunto de líneas que simulan la perspectiva (desde los bordes hacia el centro) o cargar un bitmap precalculado. Configura la copper: a menudo **un color por línea** (o cada pocas líneas) para dar gradiente de profundidad (más oscuro arriba, más claro abajo, o al revés). Activa copper y DMA.
2. **Render**: Si la imagen es estática y solo cambia la paleta, actualiza los MOVE de color en la copper según frameCount (por ejemplo rotar o pulsar los colores). Si se dibujan líneas, puede actualizar ligeramente el ángulo o la posición del “punto de fuga”. TaskWaitVBlank.
3. **Kill**: Libera bitmap y copper.

## Mecanismos de hardware que lo hacen posible

- **Copper**: Es el **corazón** del efecto si la “catedral” es sobre todo **color por línea**. La lista copper tiene un WAIT y un MOVE a COLOR00 (o a varios COLORxx) por cada línea (o cada grupo de líneas). Así se pinta un gradiente vertical sin tocar el bitmap: cada línea usa un color distinto. También puede escribir BPLCON1 por línea para un scroll horizontal suave.
- **Blitter** (si se dibujan líneas): **BlitterLine** dibuja los segmentos que forman la perspectiva. Muchas líneas desde los laterales hacia el centro dan la sensación de techo o pasillo.
- **DMA de raster**: Muestra el bitmap. Si el efecto es solo paleta, el bitmap puede ser muy simple (incluso un solo color por línea); la “riqueza” visual viene de los colores que carga el copper.
- **CPU**: Calcula las coordenadas de las líneas (perspectiva: y = k/x o similar) o los índices de color para la copper.
- **Chip RAM**: Bitmap y lista copper en chip.

## Técnicas y trucos

- **Gradiente por línea**: Una tabla de 256 colores (o menos) que van de oscuro a claro; la copper carga color[i] en la línea i. Sensación de “luz al fondo” o de profundidad.
- **Perspectiva 2D**: Las líneas van desde (x1, 0) hasta (centerX, centerY) y desde (x2, 0) hasta (centerX, centerY), etc. El “punto de fuga” está en (centerX, centerY). Fórmula: x = centerX + (x_edge - centerX) * (y / HEIGHT).
- **Bitmap estático + paleta animada**: Si el bitmap ya tiene la forma de la catedral (grises o índices), animar solo la paleta da sensación de que la luz cambia sin redibujar.

## Conceptos para principiantes

- **Perspectiva** — En 2D, simular profundidad haciendo que las líneas converjan en un “punto de fuga”. Los objetos “lejanos” se dibujan más pequeños y más cerca del centro.
- **Color por línea** — Patrón muy usado en Amiga: en la copper, para cada línea de pantalla se escribe un valor distinto en COLOR00 (o en varios). La Denise usa ese valor para esa línea; el resultado es un gradiente o una “cortina” de color sin dibujar píxeles.
- **Punto de fuga** — Punto del plano donde parecen converger todas las líneas de profundidad. En una catedral vista desde abajo, suele estar arriba al centro.
