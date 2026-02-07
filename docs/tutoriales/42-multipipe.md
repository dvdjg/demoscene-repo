# Tutorial: MultiPipe

## Objetivo

Efecto de **múltiples tuberías (pipes)** en 3D: varios cilindros o tubos que se extienden en profundidad (eje Z), a menudo con textura o color por segmento. Se dibujan como **tramos** (anillos o bandas) proyectados en pantalla; puede usar **BlitterFill** o **CpuFill** para rellenar franjas y **CpuLine** o **BlitterLine** para los bordes.

## Archivos clave

- `effects/multipipe/multipipe.c` — Init (bitmap, copper, tal vez tabla de senos), Render (para cada tubo: calcula posición 3D de cada anillo, proyecta a 2D, dibuja relleno entre anillos consecutivos o líneas de contorno).
- Si usa lib3d: proyección con ProjectPoint3D; relleno con FillQuad o similar.

## Flujo

1. **Init**: Crea bitmap y copper. Precalcula geometría de los tubos (radio, número de segmentos por anillo, número de anillos en Z). Carga paleta. Activa DMA.
2. **Render**: Para cada tubo: actualiza ángulo o posición (animación). Para cada “rebanada” en Z: calcula los puntos del anillo en 3D, proyecta a 2D (ProjectPoint3D). Con los puntos 2D se forman **trapecios** (o quads) entre anillo actual y el siguiente; se rellenan con **BlitterFill** (por bandas horizontales) o con rutina de relleno de polígono. Opcional: se dibujan las líneas de borde con CpuLine/BlitterLine. Se usa ordenamiento por Z (o por Y) para dibujar de atrás hacia delante. TaskWaitVBlank.
3. **Kill**: Libera bitmap y copper.

## Mecanismos de hardware que lo hacen posible

- **Blitter**: **BlitterFill** rellena rápidamente bandas horizontales (o rectángulos) con un color. Para cada “banda” del tubo entre dos anillos proyectados se calculan las Y mín y máx y las X izquierda/derecha (o se rellena un trapecio por líneas horizontales). El blitter hace el relleno en mucho menos ciclos que la CPU.
- **CPU (68000)**: Cálculo 3D (rotación de puntos del anillo), **proyección** (x’ = x/z o similar, y’ = y/z), ordenamiento de segmentos por profundidad. La lógica del efecto y la generación de las coordenadas 2D es en CPU.
- **Copper**: bplpt y LoadColors. La imagen es el bitmap con los tubos dibujados.
- **DMA de raster**: Muestra el bitmap.
- **Chip RAM**: Bitmap en chip para blitter y raster.

## Técnicas y trucos

- **Anillos proyectados**: Un tubo se modela como varios círculos (anillos) en planos perpendiculares a Z. Cada anillo se transforma (rotación) y se proyecta; los cuadriláteros entre anillos consecutivos se rellenan. Color o intensidad puede depender de Z (más oscuro atrás).
- **Relleno de trapecio**: Entre dos líneas (arriba y abajo del tubo) se rellenan líneas horizontales: para cada Y entre y1 y y2, calcular x_left y x_right por interpolación, luego BlitterFill desde (x_left,Y) hasta (x_right,Y).
- **Varios tubos**: Mismo código por tubo; cada uno con su posición y ángulo. Dibujar todos de atrás a delante (orden por Z del centro del tubo) para que no se tapen mal.

## Conceptos para principiantes

- **Proyección 3D→2D** — Convertir un punto (x,y,z) en el espacio en un punto (x’,y’) en pantalla. La fórmula típica es x’ = x/z * scale + center_x (y igual para y). Así los objetos lejanos (z grande) se ven más pequeños.
- **Trapecio** — Cuadrilátero con dos lados paralelos. En 3D, al proyectar dos segmentos (por ejemplo dos anillos de un tubo) se obtienen dos líneas en pantalla que forman los lados del trapecio; el relleno es entre esas dos líneas.
- **Orden de pintado (painter’s algorithm)** — Dibujar primero los objetos que están más lejos y después los cercanos, para que los cercanos tapen correctamente a los lejanos.
