# Tutorial: Stripes

## Objetivo

**Franjas** (segmentos) que se mueven en perspectiva 2D: cada franja tiene posición (y, z) y color; se proyectan a Y de pantalla (división por Z), y el **color de fondo** se cambia por línea en el copper según qué franja “toca” esa línea. Sensación de túneles o listas que se acercan.

## Archivos clave

- `effects/stripes/stripes.c` — GenerateStripes (posiciones aleatorias), GenerateColorShades (degradados), MakeCopperList (un MOVE de color por línea), RotateStripes (proyección 2D), Render (ordenar por Y proyectada y escribir colores en copper).
- Uso de SIN/COS para rotar (y,z) y div16 para proyectar (yp/z).

## Flujo

1. **Init**: GenerateStripes() rellena array de StripeT con (y, z, color) aleatorios. GenerateColorShades() crea degradados entre colores base y blanco/negro. Crea copper list: por cada línea Y un WAIT y un CopSetColor(cp, 0, …) que se dejará apuntado para modificar en Render. Activa una lista.
2. **Render**: RotateStripes() rota cada (y,z) con ángulo frameCount y proyecta a Y de pantalla (yp = (y*cos - z*sin)/z_eff, o similar). Ordena o recorre las franjas por Y proyectada; para cada línea de pantalla determina qué franja (o mezcla) corresponde y escribe ese color en la instrucción copper de esa línea. CopListRun, VBlank, alternar listas si hay doble buffer.
3. **Kill**: borra listas.

## Técnicas y trucos

- **Sin bitmap de imagen**: solo color de fondo por línea. Las “franjas” son intervalos en Y con un color; se calculan con proyección (y/z) y se vuelcan a los MOVE de la copper.
- **Proyección 2D**: punto (y,z) en “espacio” se proyecta a Y pantalla = scale * y / (z + z0). Rotación en el plano YZ con SIN/COS del ángulo; fixed-point con normfx y div16.
- **ColorTransition** — Función de libgfx que interpola entre dos colores RGB (0–15 o 0–255); sirve para degradados y para suavizar el color entre franjas.
- **Copper: un color por línea** — Patrón mínimo para efectos “por línea”: 256 WAIT+MOVE, cada MOVE es el COLOR00 (o el que sea) para esa línea.

## Conceptos para principiantes

- **Proyección perspectiva simple** — En 2D, si la cámara está en (0,0) y el punto en (y,z), la proyección en “pantalla” es proporcional a y/z. Cuanto mayor es z, más “lejos” y más cerca del centro.
- **Ordenación por Y** — Para pintar de atrás adelante (o para saber qué franja está delante en cada línea), se ordenan las franjas por su Y proyectada antes de rellenar los colores.
