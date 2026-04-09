# Guía CI y agentes (amiga-engine)

## Señales observables

Los agentes automatizados deben basarse en **fallos binarios**:

| Comando | Criterio de éxito |
|---------|-------------------|
| `make -C amiga-engine host-test` | Exit code 0, líneas TAP `ok` para todos los tests planificados |
| `make -C effects/engine-demo` | Produce `engine-demo.exe` sin errores de enlace |

## Formato TAP

`tests-host-runner` imprime `TAP version 13`, la línea `1..N` y una línea `ok` / `not ok` por aserción. Los parsers pueden contar `not ok`.

## Reproducir en emulador

Tras `make -C effects/engine-demo run`, usar el ADF generado con `fs-uae` y la configuración del README principal del repositorio.

## Iteración automática

1. Ejecutar tests host tras cada cambio en `include/amiga_engine` o `src/cxx`.
2. Si un test de copper falla, comparar palabras generadas por `CopperProgramBuilder::encode_words` con el valor esperado en el propio test.
