# Tutorial: Floor-old

## Objetivo

Versión **antigua** del efecto Floor: mismo tipo de idea (suelo con franjas en perspectiva, sensación de profundidad) pero con otra implementación: más pasadas, otra forma de generar la tabla de BPLCON1 o de colores por franja. Sirve para comparar técnicas y ver cómo se puede hacer el mismo efecto de varias maneras.

## Archivos clave

- `effects/floor-old/floor-old.c` — Init (Load puede precalcular tablas), MakeCopperList, Render (actualización de franjas, colores, BPLCON1).
- Datos: floor, stripes, stripeWidth, stripeLight (similares a floor).

## Flujo

1. **Load** (opcional): Precalcula tablas (anchos de franja por línea, niveles de luz, valores de BPLCON1) para no hacerlo en Init.
2. **Init**: Configura playfield, crea copper list con **un WAIT y uno o varios MOVE por línea** (BPLCON1 para scroll horizontal, y/o COLOR para las franjas). Usa el bitmap estático del suelo (floor). Activa copper y DMA.
3. **Render**: Actualiza el “offset” de las franjas (scroll). Escribe en las instrucciones copper los valores de BPLCON1 por línea (desde una tabla) y los colores por franja (según stripeLight y el offset). CopListRun, VBlank, alternar lista si hay doble buffer.
4. **Kill**: Libera listas y desactiva DMA.

## Mecanismos de hardware que lo hacen posible

- **Copper**: Igual que en Floor: es quien **cambia BPLCON1 por línea** (scroll horizontal fino) y **carga los colores** de cada franja. Sin copper no se podría tener un valor distinto por scanline; el efecto depende de eso.
- **DMA de raster**: Muestra el bitmap del suelo. El bitmap no se modifica en Render; solo la interpretación (scroll y paleta) cambia vía copper.
- **CPU**: Calcula qué valor de BPLCON1 y qué color corresponde a cada línea (usando tablas precalculadas o fórmulas) y escribe esos valores en las instrucciones MOVE de la copper.
- **Chip RAM**: Bitmap floor y listas copper en chip.

La diferencia con Floor actual suele estar en **cómo** se rellenan las tablas, cuántas instrucciones copper hay por línea, o si se usa una o dos listas; el hardware usado es el mismo.

## Técnicas y trucos

- **Tablas precalculadas**: En Load o Init se genera shifterValues[offset][scanline] y se usa en Render para escribir en la copper. Así Render solo hace “lookup” y escritura, no cálculos pesados.
- **Comparar con Floor**: Ver floor.c y floor-old.c en paralelo ayuda a entender qué optimizaciones se hicieron (menos MOVE, menos divisiones, tabla más compacta, etc.).

## Conceptos para principiantes

- **BPLCON1** — Registro que controla el scroll horizontal fino (y otras opciones) del playfield. Escribirlo en cada línea desde el copper hace que cada línea “desplace” sus píxeles un número de píxeles distinto; eso da la sensación de perspectiva en un bitmap estático.
- **Versión “old”** — En muchos proyectos se conserva una versión anterior de un efecto para referencia o para no perder una técnica que luego se simplificó. Comparar ambas enseña evolución del código.
