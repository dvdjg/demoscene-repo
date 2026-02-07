# Tutorial: Ball

## Objetivo

Una **bola** (o sprite de bola) que **rebota** por la pantalla. La posición se actualiza cada frame (física simple: velocidad, rebote en los bordes) y la bola se dibuja en el bitmap con el blitter.

## Archivos clave

- `effects/ball/ball.c` — Init (bitmap, datos de la bola), Render (actualizar posición, comprobar bordes, dibujar bola).
- `effects/ball/data/gen-ball.py` — Genera el gráfico de la bola (círculo o sprite) y lo exporta a datos C.

## Flujo

1. **Init**: Crea bitmap(s) de pantalla (doble buffer si se usa). Carga el bitmap de la bola (desde el .c generado por gen-ball.py). Configura playfield, paleta y copper (CopSetupBitplanes). Inicializa posición (x, y) y velocidad (vx, vy) de la bola.
2. **Render**: Borra la zona donde estaba la bola en el frame anterior (o se redibuja todo el fondo). Actualiza posición: x += vx, y += vy. Si x o y salen de los límites, se invierte la velocidad (vx = -vx o vy = -vy) y se corrige la posición para que no se salga. **BitmapCopy** (o BitmapCopyArea) del bitmap de la bola a (x, y) en el bitmap de pantalla. Actualiza bplpt en la copper, TaskWaitVBlank.
3. **Kill**: Libera bitmaps y copper.

## Mecanismos de hardware que lo hacen posible

- **Blitter**: **BitmapCopy** (o BitmapCopyArea) mueve el rectángulo de la bola desde el gráfico precalculado al bitmap de pantalla. El blitter hace la copia de todos los planos en paralelo; la CPU solo programa origen, destino y tamaño.
- **Copper**: Mantiene los punteros de bitplanes (bplpt) para que la Denise muestre el bitmap donde hemos dibujado. Si hay doble buffer, el copper apunta al buffer que acabamos de rellenar.
- **DMA de raster**: Cada línea de pantalla la Denise la lee desde chip RAM usando los bplpt. Sin este DMA no se vería la imagen.
- **Chip RAM**: Tanto el bitmap de la bola como el de pantalla deben estar en chip memory; el blitter solo puede escribir ahí y el raster solo lee de ahí.
- **CPU**: Calcula la nueva posición y las colisiones con los bordes (sumas y comparaciones). Es poco trabajo comparado con el dibujo.

## Técnicas y trucos

- **Física mínima**: Sin gravedad: velocidad constante; solo se cambia el signo de vx o vy al tocar un borde. Con gravedad: vy += 1 cada frame y se rebota en el “suelo”.
- **Evitar borrar toda la pantalla**: Para no parpadear, se puede borrar solo el rectángulo donde estaba la bola y luego dibujar la bola en la nueva posición. Eso requiere guardar el “fondo” bajo la bola o redibujar el fondo en esa zona.
- **Bola como bitmap pequeño**: gen-ball.py genera un círculo (o elipse) en un bitmap de tamaño fijo; ese bitmap se copia una y otra vez en (x, y). La máscara (transparencia) se puede hacer con BlitterCopyMasked si el fondo no es uniforme.

## Conceptos para principiantes

- **BitmapCopy(dst, x, y, src)** — Copia el bitmap completo `src` a la posición (x, y) en `dst`. El blitter hace el trabajo; hay que esperar (WaitBlitter) si luego se usa el blitter de nuevo.
- **Doble buffer**: Se dibuja en un bitmap que aún no se muestra; al terminar el frame se cambia la copper para que bplpt apunte a ese bitmap. Así el usuario nunca ve el “lapiz” a medias.
- **Rebote**: Si la coordenada sale del rango [min, max], se hace velocidad = -velocidad y se vuelve a meter la posición dentro del rango (pos = min o max) para no quedarse fuera.
