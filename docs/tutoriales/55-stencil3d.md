# Tutorial: Stencil3D (Estarcido 3D)

## Objetivo

Efecto **stencil 3D**: dibujar objetos 3D (o una escena) usando una técnica de **estarcido** (stencil): primero se dibuja una **máscara** (silueta o profundidad) y luego se rellena o se dibuja **solo donde la máscara lo permite**. En Amiga puede hacerse con **varios planos de bit**: un plano (o varios) para la “máscara” y el resto para el contenido; o con **blitter** usando minterms (AND/OR) para combinar máscara y relleno. Suele aplicarse a **sombras** o a **efectos de “recorte”** sobre un fondo.

## Archivos clave

- `effects/stencil3d/stencil3d.c` — Init (bitmap, copper, geometría 3D o modelo), Render (dibuja la silueta/máscara del objeto 3D en un plano o buffer, luego rellena o copia textura “donde” la máscara es 1).
- Posible uso de BlitterCopy con máscara o de dos pasadas (máscara + relleno).

## Flujo

1. **Init**: Crea bitmap (varios planos si hace falta: unos para máscara, otros para imagen). Configura copper. Carga geometría 3D (vértices, caras) o define un objeto (esfera, toro). LoadColors. Activa DMA.
2. **Render**: **Pasada 1 (máscara)**: Dibuja la **silueta** del objeto 3D proyectado (contorno o relleno de la proyección) en el plano de máscara (o en un buffer). Por ejemplo: proyectar todas las caras, rellenar los polígonos con 1 en el plano de máscara. **Pasada 2 (contenido)**: Con el blitter, **copia o rellena** la imagen o color **solo donde la máscara es 1** (usando minterm que haga dest = dest OR (source AND mask), o dibujando solo en las zonas con máscara). Opcional: sombra = misma máscara desplazada y con color oscuro. TaskWaitVBlank.
3. **Kill**: Libera bitmap y copper.

## Mecanismos de hardware que lo hacen posible

- **Blitter**: **Minterms** permiten combinar hasta tres fuentes (A, B, C) con operaciones lógicas (AND, OR, etc.). Para stencil: A = máscara, B = imagen o color a poner. Por ejemplo: destino = (dest AND NOT A) OR (B AND A): donde la máscara es 1 se pone B, donde es 0 se deja el destino. BlitterCopy con máscara (modo que use el canal A como máscara) hace esto en una pasada. El blitter es muy rápido para estas operaciones en chip RAM.
- **CPU**: **Proyección 3D** del objeto (vértices a 2D), **generación de la máscara** (relleno de polígonos proyectados en el plano de máscara) o contorno. Orden de caras si hace falta.
- **Copper**: bplpt para todos los planos (máscara + imagen). LoadColors. Si la máscara y la imagen comparten el mismo bitmap en planos distintos, el raster muestra la combinación.
- **DMA de raster**: Lee todos los planos; el color final es el índice de los bits de cada plano. La “máscara” puede ser un plano que solo afecta a qué color se muestra (por ejemplo prioridad) o usarse solo en la fase de dibujo con blitter.
- **Chip RAM**: Bitmap y máscara en chip para blitter.

## Técnicas y trucos

- **Dos planos**: Plano 0 = máscara (0 o 1). Planos 1–4 = imagen. Primero limpiar planos 1–4; luego “donde plano 0 es 1”, escribir en 1–4 con la imagen. Se puede hacer con blitter: copiar imagen usando el plano 0 como control (solo escribir donde A=1).
- **Sombra**: La máscara del objeto proyectada al “suelo” (Y fijo, X+Z) se dibuja en el plano de sombra; se rellena con color oscuro. Luego se dibuja el objeto encima. Da sensación de objeto con sombra en el suelo.
- **Minterm** — Fórmula de 3 fuentes (A,B,C) con AND/OR/NOT. El manual del blitter lista las minterms; elegir la que implemente “donde mask=1, dest=source”.

## Conceptos para principiantes

- **Stencil (estarcido)** — En gráficos: una máscara que dice “solo dibuja aquí”. Se usa para recortar una forma (como un círculo o la silueta de un personaje) sobre un fondo.
- **Minterm del blitter** — Fórmula que combina los canales A, B y C del blitter con operaciones lógicas. Por ejemplo (A AND B) OR (C AND NOT A). Permite efectos de máscara y mezcla sin hacer varios pasadas en CPU.
- **Proyección 3D** — Pasar los vértices del objeto 3D a 2D (x’, y’) para dibujar la silueta en el bitmap. La máscara es “qué píxeles pertenecen al objeto” en esa proyección.
