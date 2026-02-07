# Tutorial: FlatShade-convex

## Objetivo

**Shading plano** (una luz por cara) sobre un objeto 3D que se sabe **convexo** (no tiene huecos; cualquier línea entre dos puntos del objeto está dentro del objeto). Gracias a eso **no hace falta ordenar las caras** por Z: se pueden dibujar en cualquier orden y el resultado es correcto. Ahorra tiempo de CPU respecto a FlatShade.

## Archivos clave

- `effects/flatshade-convex/flatshade-convex.c` — Init (Object3D, bitmap, copper), Render (transformación, UpdateFaceVisibility, dibujo de caras **sin** SortFaces).
- `data/pilka.mtl` — Malla convexa (por ejemplo esfera).

## Flujo

1. **Init**: Igual que FlatShade: Object3D desde pilka, bitmap, copper, playfield, paleta.
2. **Render**: **UpdateObjectTransformation**, **UpdateFaceVisibility** (cada cara tiene su color 0–15). **No** se llama a SortFaces. Se dibujan las caras en el orden que vienen en la malla (o en cualquier orden); como el objeto es convexo, las que están “delante” en pantalla se dibujan después y tapan a las de atrás sin necesidad de ordenar. Se rellena cada triángulo con BlitterLine + BlitterFill. TaskWaitVBlank.
3. **Kill**: Libera objeto, bitmap y copper.

## Mecanismos de hardware que lo hacen posible

- **CPU (68000)**: Hace **transformación 3D** y **UpdateFaceVisibility** (normal·luz → color). **No** hace la ordenación (SortFaces), lo que ahorra tiempo y código. Sigue usando fixed-point (lib3d).
- **Blitter**: Dibuja los **bordes** del triángulo (BlitterLine) y **BlitterFill** para el relleno. Igual que en FlatShade.
- **Copper**: bplpt y paleta. La paleta tiene los 16 niveles de luz.
- **DMA de raster**: Muestra el bitmap.
- **Chip RAM**: Malla y bitmap en chip.

**Por qué no hace falta SortFaces**: En un objeto convexo, las caras no se atraviesan entre sí desde ningún punto de vista. Si dibujas todas las caras en cualquier orden, las que están más cerca de la cámara acaban “encima” en el bitmap porque se dibujan después y escriben sobre las que ya estaban. En un objeto no convexo (con agujeros o partes que se solapan) sí habría que ordenar por Z para que no se vean triángulos por detrás de otros.

## Técnicas y trucos

- **Convexo = orden arbitrario**: Comprobar que la malla es convexa (por ejemplo una esfera, un cubo, un cilindro cerrado). Entonces se puede dibujar en el orden del array de caras sin SortFaces.
- **Misma pipeline que FlatShade**: UpdateObjectTransformation, UpdateFaceVisibility, relleno de triángulos. Solo se quita la llamada a SortFaces y la gestión de visibleFace.
- **Pilka**: Malla de “pelota”; típicamente una esfera generada por obj2c, que es convexa.

## Conceptos para principiantes

- **Objeto convexo** — Un sólido es convexo si, para cualquier par de puntos dentro del sólido, el segmento que los une está totalmente dentro. Ejemplos: esfera, cubo, elipsoide. Una donut (toro) no es convexa porque hay un “agujero”.
- **Painter’s algorithm** — Pintar de atrás adelante. En convexos no hace falta ordenar porque “atrás” y “adelante” no se cruzan; en no convexos sí.
- **SortFaces** — Ordena las caras por su Z medio para saber en qué orden dibujarlas. Coste proporcional al número de caras; en convexos se evita.
