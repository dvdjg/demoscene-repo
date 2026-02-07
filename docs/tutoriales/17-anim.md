# Tutorial: Anim

## Objetivo

Mostrar una **animación** formada por una secuencia de imágenes (frames) pregeneradas. Cada frame es un bitmap; el efecto avanza de frame en frame y muestra el que corresponde al tiempo actual.

## Archivos clave

- `effects/anim/anim.c` — Init (carga o prepara frames), Render (elige frame según frameCount y lo muestra).
- `effects/anim/data/gen-anim.py` — Script que genera los datos de los frames (por ejemplo, exportar desde imágenes o desde otro formato).

## Flujo

1. **Init**: Reserva bitmaps o carga los datos de la animación (array de planos o de imágenes). Configura playfield y copper (CopSetupBitplanes). La animación puede estar en un único bitmap grande (cada “frame” es una región) o en varios buffers.
2. **Render**: Calcula el índice de frame a partir de **frameCount** (y a veces la velocidad de animación). Si cada frame es una región distinta, actualiza los punteros de bitplanes (bplpt) para que apunten al inicio de ese frame en memoria; si es un bitmap distinto, copia ese frame al bitmap visible con el blitter. Espera VBlank.
3. **Kill**: Libera bitmaps y datos de la animación.

## Mecanismos de hardware que lo hacen posible

- **Copper**: Escribe en los registros **bplpt** (punteros de bitplanes). Si los frames están uno detrás de otro en memoria, solo hay que cambiar bplpt para “saltar” al siguiente frame sin copiar datos. El copper puede hacerlo al inicio de cada frame (en VBlank).
- **DMA de raster**: La Denise usa los valores actuales de bplpt para leer la imagen. Al cambiar bplpt (por código o por copper), la siguiente imagen que se dibuja es la nueva.
- **Blitter (si se copia frame a frame)**: Si en vez de cambiar bplpt se copia el frame actual a un buffer de pantalla, el **blitter** hace la copia de todos los planos; la CPU solo tiene que decir origen, destino y tamaño.
- **Chip RAM**: Los datos de todos los frames deben estar en chip memory para que el DMA de video (o el blitter) los use.

## Técnicas y trucos

- **Animación por cambio de punteros**: La forma más barata es tener todos los frames en un bloque de memoria y solo cambiar bplpt al inicio de cada frame. No se mueve ningún píxel; solo se dice “de dónde leer”.
- **Velocidad de animación**: Dividir frameCount entre un número (por ejemplo `frameCount / 2`) para que la animación vaya más lenta (un frame nuevo cada 2 refrescos).
- **Bucle**: Si el número de frames es N, el índice se calcula como `frameCount % N` para que la animación se repita.

## Conceptos para principiantes

- **frameCount** — Variable global (en effect.h) que cuenta cuántos frames han pasado desde que empezó el efecto. A 50 Hz (PAL), cada segundo son 50 frames.
- **bplpt (BPL1PT, BPL2PT, …)** — Registros que guardan la dirección de memoria donde empieza cada plano de bits. La Denise lee la imagen desde ahí. Cambiar esta dirección equivale a “cambiar de imagen” sin tocar píxeles.
- **Pregenerar datos** — En demos es muy común generar los gráficos o tablas con un script (Python, etc.) y convertirlos a .c para compilarlos con el efecto. Así la CPU del Amiga no tiene que calcular la animación en tiempo real.
