# Tutorial: Wireframe

## Objetivo

Mostrar un **objeto 3D** (por ejemplo una esfera o malla) solo como **aristas** (líneas), sin rellenar caras. El objeto rota; las caras que no se ven (back-face) no dibujan sus aristas.

## Archivos clave

- `effects/wireframe/wireframe.c` — Init (objeto 3D, bitmap, copper), Render (transformar, visibilidad, dibujar aristas), Kill.
- `include/3d.h`, `lib/lib3d/*` — Object3D, NewObject3D, transformaciones, visibilidad, SortFaces.
- `data/pilka.c` — Malla (vértices, caras) generada con obj2c.
- `data/wireframe-pal.c` — Paleta.

## Flujo

1. **Init**: crea un Object3D desde la malla (pilka), pone translate.z para alejarlo de cámara. Crea bitmap y copper con CopSetupBitplanes, SetupPlayfield, LoadColors, activa DMA (blitter, raster).
2. **Render**:  
   - Actualiza ángulos de rotación del objeto (p. ej. en función de frameCount).  
   - **UpdateObjectTransformation(object)** — Calcula matrices objectToWorld y worldToObject y posición de cámara en espacio objeto.  
   - **TransformVertices** — Proyecta vértices 3D a 2D (perspectiva): multiplica por objectToWorld y divide por Z (o similar); escribe en vertex[].  
   - **UpdateFaceVisibilityFast** — Para cada cara, producto escalar normal·vectorHaciaCámara; si &lt; 0 la cara no se ve y se marca face->flags = -1.  
   - **UpdateEdgeVisibility** — Para cada cara visible, marca sus aristas y vértices como visibles (flags en NODE3D y EDGE).  
   - Dibuja solo las aristas con flags &gt;= 0: para cada arista, si está visible, **BlitterLine(x1,y1, x2,y2)** entre los dos vértices proyectados.  
   - (Opcional) limpia el bitmap antes del siguiente frame.  
3. **Kill**: borra objeto, bitmap y copper.

## Técnicas y trucos

- **Back-face culling**: solo se dibujan aristas de caras cuyo producto escalar (normal · vista) ≥ 0. Así no se ven las líneas del “revés” del objeto.
- **Proyección en fixed-point**: la proyección perspectiva (x/z, y/z) se hace con enteros; típicamente se usa un factor de escala y división por Z (o normfx). Las macros MULVERTEX1/MULVERTEX2 en el código optimizan la multiplicación matriz×vértice y el paso a 2D.
- **BlitterLine**: en lugar de dibujar con CPU (Bresenham), se usa el blitter en modo línea; es más rápido y libera CPU. BlitterLineSetup(screen, plane, LINE_OR o LINE_EOR) y luego BlitterLine(x1,y1,x2,y2).
- **Malla estática**: los vértices y la lista de caras/aristas vienen del .c generado por obj2c; no se modifican en tiempo de ejecución, solo se transforman y proyectan.
- **Un solo plano para líneas**: si se usa un solo bitplane para el wireframe, solo hay dos “colores” (línea sí/no). Con más planos se pueden colorear por profundidad o por cara.

## Conceptos para principiantes

- **Object3D / Mesh3D** — La malla tiene vértices (Point3D), caras (Face con normal, índices a vértices) y aristas (Edge con dos vértices). Object3D añade transformación (rotate, scale, translate), matrices y la lista de caras visibles ordenadas por Z.
- **objectToWorld** — Matriz que lleva puntos del modelo a coordenadas de mundo. Se construye con LoadRotate3D, Scale3D, Translate3D.
- **Back-face culling** — Eliminar caras que miran hacia atrás (normal alejándose de cámara) para no dibujarlas. En wireframe se usa para no dibujar las aristas de esas caras.
- **Blitter en modo línea** — El blitter puede dibujar líneas (Bresenham en hardware); se configura con BlitterLineSetup y se llama BlitterLine con los extremos. Más rápido que hacerlo en CPU.
