# Tutorial: Shapes

## Objetivo

Dibujar **formas 2D** (polígonos) que rotan y escalan en pantalla. Las formas están definidas por listas de puntos y polígonos (ShapeT); se transforman con una matriz 2D, se recortan contra la ventana (ClipPolygon2D) y se dibujan con el **blitter** (líneas + relleno).

## Archivos clave

- `effects/shapes/shapes.c` — Init (ClipWin, bitmap, copper), Render (matriz, Transform2D, PointsInsideBox, ClipPolygon2D, DrawShape, BlitterFill).
- `include/2d.h`, `lib/lib2d/*` — Matrix2D, LoadIdentity2D, Rotate2D, Scale2D, Translate2D, Transform2D, PointsInsideBox, ClipPolygon2D.
- `data/shapes-pal.c`, `data/night.c` — Paleta y forma (shape con origPoint, polygon).

## Flujo

1. **Init**: crea bitmap 320×256×4, define **ClipWin** (0,0)–(319,255) para el recorte. Habilita blitter, limpia pantalla, SetupPlayfield, LoadColors, copper con CopSetupBitplanes, activa DMA.
2. **Render**:  
   - Limpia el plano donde se dibujará (BlitterClear(screen, plane)).  
   - Construye matriz 2D: LoadIdentity2D, Rotate2D(angle), Scale2D(sx, sy) — con sx/sy que pulsan usando SIN/COS —, Translate2D(center).  
   - **Transform2D(&t, shape.viewPoint, shape.origPoint, shape.points)** — Aplica la matriz a todos los puntos; resultado en viewPoint.  
   - **PointsInsideBox(shape.viewPoint, shape.viewPointFlags, shape.points)** — Marca qué puntos están dentro de ClipWin (para recorte rápido).  
   - **BlitterLineSetup(screen, plane, LINE_EOR | LINE_ONEDOT)** — Modo EOR y un punto por píxel para el contorno.  
   - **DrawShape(&shape)** — Para cada polígono: si no todos los vértices están fuera (outside), llama a **ClipPolygon2D** (Sutherland-Hodgman) y luego **DrawPolygon** (recorre aristas y BlitterLine entre vértices consecutivos).  
   - **BlitterFill(screen, plane)** — Rellena el interior del contorno cerrado (el blitter tiene modo fill que usa el estado de los bits del borde).  
   - Actualiza los punteros de bitplanes en la copper (rotación de planos para efecto de “depth”) y TaskWaitVBlank. Alterna el plano de dibujo cada dos frames.
3. **Kill**: desactiva DMA, borra copper y bitmap.

## Técnicas y trucos

- **Lib2D al completo**: identidad, rotación, escala, traslación y Transform2D en fixed-point; luego recorte con PointsInsideBox y ClipPolygon2D. Ideal para aprender la pipeline 2D del repo.
- **Recorte de polígonos (Sutherland-Hodgman)**: ClipPolygon2D recorta el polígono contra las 4 aristas de ClipWin (izquierda, arriba, derecha, abajo). Así los polígonos que salen de pantalla no generan líneas fuera de límites.
- **Dibujo por líneas + relleno**: primero se dibujan las aristas del polígono con BlitterLine en modo EOR; luego BlitterFill rellena usando la paridad de los cruces (algoritmo de relleno del blitter). Muy eficiente en Amiga.
- **Escala animada**: Scale2D usa `fx12f(1.0) + SIN(a)/2` (y similar con COS) para que la forma “pulse” sin cambiar la geometría base.
- **Rotación de planos**: cada frame se muestran los planos en orden distinto (plane, plane+1, …) para dar sensación de profundidad o parpadeo controlado.

## Conceptos para principiantes

- **ClipWin** — Variable global Box2D (minX, minY, maxX, maxY). Todas las rutinas de recorte de lib2d (ClipLine2D, ClipPolygon2D, PointsInsideBox) usan esta ventana.
- **LINE_EOR** — El blitter en modo línea puede hacer OR o EOR con el contenido del plano. EOR permite “invertir” y luego rellenar por paridad.
- **BlitterFill** — Modo del blitter que rellena regiones delimitadas por líneas ya dibujadas en el plano; no hay que rellenar polígono a mano en CPU.
- **ShapeT** — Estructura con origPoint (puntos originales), viewPoint (transformados), viewPointFlags (dentro/fuera de ClipWin), polygon (listas de índices de vértices por polígono).
