# Tutorial: Darkroom

## Objetivo

Efecto de **“cuarto oscuro”**: como revelar una foto. La imagen (o una silueta) aparece poco a poco, o la **paleta** se va iluminando/oscureciendo para dar sensación de revelado o de contraste que cambia.

## Archivos clave

- `effects/darkroom/darkroom.c` — Init (bitmap, imagen base, copper), Render (actualiza paleta o “nivel de revelado” según frameCount).
- Datos: imagen en escala de grises o bitmap precalculado y paleta(s).

## Flujo

1. **Init**: Carga o genera el bitmap que se “revelará” (puede ser una foto en grises o una silueta). Configura la paleta inicial (muy oscura: todos los índices apuntan a negro o casi negro). La copper puede cargar esa paleta al inicio del frame; durante el efecto se irán cambiando los valores de los COLORxx hacia los colores “reales”.
2. **Render**: Cada frame se avanza el “nivel de revelado”: por ejemplo, se interpolan los colores de la paleta desde (0,0,0) hacia el color final (ColorTransition o similar). Se escriben los nuevos valores en la copper (en las instrucciones MOVE que cargan COLOR00–COLOR15) o con SetColor. TaskWaitVBlank.
3. **Kill**: Libera bitmap y copper.

## Mecanismos de hardware que lo hacen posible

- **Copper**: Permite **cambiar la paleta cada frame**. Los registros COLOR00–COLOR31 definen “qué color se ve” para cada índice de píxel. Si la imagen ya está en el bitmap (con índices 0–15) y nosotros vamos pasando de “todos los índices = negro” a “índice 1 = gris claro, índice 2 = gris medio…”, la misma imagen parece “revelarse” sin tocar un solo píxel del bitmap.
- **DMA de raster**: Muestra el bitmap. Los píxeles no cambian; lo que cambia es la traducción índice→color que hace la Denise usando los registros de color.
- **CPU**: Calcula la interpolación (de negro al color final) para cada entrada de la paleta según el “tiempo” (frameCount). Puede usar una tabla precalculada (frame → paleta completa) para no calcular en tiempo real.
- **Chip RAM**: Bitmap y lista copper en chip.

## Técnicas y trucos

- **Revelado = solo paleta**: El bitmap contiene la imagen en grises (índices 0–15). Al principio todos los COLORxx = 0x000 (negro). Cada frame se acercan un poco al color objetivo (interpolación lineal o con curva). Al final la paleta tiene los grises “reales” y la imagen se ve completa.
- **ColorTransition** — Función de libgfx que, dado dos colores RGB y un paso 0–15, devuelve el color intermedio. Muy útil para “fundir” de negro a blanco o entre dos paletas.
- **Curva de revelado**: En lugar de lineal (t/N), usar una curva (por ejemplo t²) hace que el revelado empiece lento y acelere, o al revés, más “cinematográfico”.

## Conceptos para principiantes

- **Paleta (COLOR00–COLOR31)** — El bitmap no guarda “rojo, verde, azul” por píxel; guarda un **índice** (0–31 con 5 planos). El color que se ve es el valor actual del registro COLORxx correspondiente. Cambiar el registro cambia el color de todos los píxeles con ese índice.
- **Interpolación** — Calcular valores intermedios entre A y B según un parámetro t (0…1): resultado = A + (B - A) * t. Así se pasa de “todo negro” a “colores finales” de forma suave.
- **Revelado químico** — En fotografía real la imagen aparece poco a poco en el papel. Aquí lo simulamos solo cambiando la traducción índice→color en el tiempo.
