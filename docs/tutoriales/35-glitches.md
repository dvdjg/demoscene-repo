# Tutorial: Glitches

## Objetivo

Varios **tipos de glitches** en un mismo efecto: corrupción de imagen, “tearing” (desplazamiento de líneas), colores incorrectos, etc. Los datos de **dónde** y **cuándo** aplicar cada glitch pueden venir de un script (gen_tearing.py) que genera tablas o patrones.

## Archivos clave

- `effects/glitches/glitches.c` — Init (bitmap, copper, carga de datos de glitches), Render (según frameCount o tabla, aplica uno o varios glitches: escribe en copper, bplcon1 por línea, o en el bitmap).
- `data/gen_tearing.py` — Genera datos para el efecto de tearing (qué líneas desplazar, cuánto, en qué frames).

## Flujo

1. **Init**: Crea bitmap con imagen base. Carga los datos generados por gen_tearing.py (por ejemplo: array de “en frame F, en líneas L1–L2, escribe valor V en bplcon1”). Configura copper; puede tener muchas instrucciones (un MOVE de bplcon1 por línea) que en “modo normal” tienen valor 0 y en “modo glitch” se sobrescriben con valores de la tabla. Activa DMA.
2. **Render**: Calcula el “estado” del efecto según frameCount (por ejemplo: cada 60 frames hay 5 frames de glitch). Durante los frames de glitch: escribe en las instrucciones copper los valores de la tabla (bplcon1 por línea, o colores). Durante los frames normales: restaura valores 0 o la paleta correcta. TaskWaitVBlank.
3. **Kill**: Libera bitmap, datos y copper.

## Mecanismos de hardware que lo hacen posible

- **Copper**: Si el glitch es **por línea**, la copper tiene un WAIT y un MOVE (por ejemplo a bplcon1) por cada línea. En tiempo normal todos los MOVE tienen el mismo valor (0). Para el glitch, la **CPU** modifica esas instrucciones (el campo .data del MOVE) con valores que desplazan cada línea un número de píxeles distinto = “tearing”. El copper ejecuta esa lista ya modificada en el siguiente frame.
- **CPU**: Lee la tabla generada por gen_tearing.py y escribe en la lista copper (o en custom->bplcon1 en momentos concretos). También puede escribir en el bitmap (bytes aleatorios en franjas).
- **DMA de raster**: Muestra el resultado; si la copper carga bplcon1 distintos por línea, la Denise lee los bitplanes “desplazados” de forma distinta en cada línea = imagen rota.
- **Chip RAM**: Bitmap, lista copper y tablas de glitch en chip (si la copper debe leer datos desde ahí).

## Técnicas y trucos

- **Tabla precalculada**: gen_tearing.py genera “en frame N, líneas Y1–Y2, valor V” para que el efecto sea reproducible y controlado. En el Amiga solo se hace lookup y escritura.
- **Modificar la copper en RAM**: La lista copper está en RAM; cada instrucción MOVE tiene un campo “dato”. Cambiar ese dato (CopInsSet16 o similar) hace que la siguiente vez que el copper ejecute esa instrucción escriba el valor nuevo. Así se puede tener una lista “plantilla” y rellenarla cada frame con los valores del glitch.
- **Combinar varios glitches**: Un frame = solo colores; otro frame = solo bplcon1; otro = corrupción del bitmap. Rotar entre ellos da variedad.

## Conceptos para principiantes

- **Tearing** — En video, “tearing” es cuando unas líneas muestran un frame y otras el siguiente (desplazamiento horizontal). En Amiga se simula escribiendo BPLCON1 distinto por línea para que cada línea lea los bitplanes “desplazados”.
- **Copper en RAM** — La lista de instrucciones del copper está en memoria. El programa puede modificar los datos de esas instrucciones (los valores que se van a escribir en los registros) antes de que el copper las ejecute.
- **Gen_tearing.py** — Script que decide “qué glitch en qué momento”. El Amiga no decide al azar; sigue la “partitura” generada en el PC para sincronizar con la música o el ritmo.
