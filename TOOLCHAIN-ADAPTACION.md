# Adaptación del proyecto al toolchain Bartman (Windows / ELF)

Este documento resume **qué cambió** en el repositorio para compilar con la extensión **Amiga C/C++ Compile, Debug & Profile** (BartmanAbyss): **`m68k-amiga-elf`**, salida **ELF** y conversión a Hunk con **`elf2hunk`**. Complementa [TOOLCHAIN.md](TOOLCHAIN.md), que describe uso diario y variables.

---

## Dos líneas de compilación (no son la misma “familia” con otra versión)

| Aspecto | **demoscene-toolchain** (`activate`, Linux) | **Bartman** (`activate-barto`, típ. Windows) |
|--------|---------------------------------------------|-----------------------------------------------|
| Triplete | `m68k-amigaos` | `m68k-amiga-elf` |
| Formato de objeto | Amiga **Hunk** (directo) | **ELF32** (`elf32-m68k`) → Hunk vía `elf2hunk` |
| Driver de enlace | `m68k-amigaos-ld` con flags Amiga | **`gcc` como linker** (`-nostdlib`, script `amiga-elf.lds`) |
| C → ensamblador | GCC → **GNU as** (gas) | Igual idea, pero **otra versión/target** del GCC |
| `.asm` (vasm) | `vasm -Fhunk` | `vasm` **`-Felf`**, posproceso de símbolos |
| Convención C ↔ asm | Registros en prototipos (`-mregparm`), símbolos con hábitos AmigaOS | **Sin** `-mregparm`; símbolos globales C **sin barra bajo** inicial típica de ELF |
| Preprocesador | `m68k-amigaos-cpp` | `m68k-amiga-elf-gcc -E` |

No es solo “una versión más nueva del mismo compilador”: es **otro backend** (AmigaOS freestanding vs ELF), **otro formato** de objetos y **otro flujo de enlace**. Por eso aparecen diferencias en:

- **nombres de símbolos** (p. ej. `ExcVecBase` vs `_ExcVecBase`, `MemAlloc` vs `_MemAlloc`);
- **`objcopy`**: salida `-O amiga` no es válida para el `objcopy` del cruce ELF → hay que usar **`elf32-m68k`** y, a veces, `--redefine-sym` hacia los nombres que el C espera;
- **asm inline en C**: el GCC emite **sintaxis GNU as**; mezclar mnemónicos “motorola compactos” (`movel`, `addaw`) con registros sin `%` (`d0`) rompe el ensamblado. Hay que usar **`move.l` / `adda.w` / `%%d0`** (etc.) donde corresponda;
- **vasm en ELF**: `xdef _Foo` exporta **`_Foo`**; el C referencia **`Foo`** → script **`tools/elf-rename-vasm-syms.sh`** y, en casos puntuales, **`--defsym`** en el enlazador.

---

## Cambios principales en el repositorio (por categoría)

### Build y flags (`build/common.mk`, `build/effect.mk`)

- Detección **`TOOLCHAIN_ELF`** cuando `M68K_TRIPLET` es `m68k-amiga-elf`.
- **`CC`/`LD` unificados** en `gcc`; **`RANLIB` → `ar s`** (kit Windows sin `ranlib.exe`).
- **`VASM`**: `vasmm68k_mot` y **`-Felf -dwarf=3`**; **`-I` al `sys-include`** del NDK del Bartman (`…/m68k-amiga-elf/sys-include`) para `include 'exec/types.i'` en `.asm`.
- **`LDFLAGS`**: `--gc-sections`, `--orphan-handling=error`, script **`system/amiga-elf.lds`**; alias de símbolos para ensamblador/C:
  - `_ExcVecBase` → `ExcVecBase` (libpt);
  - `_MemAlloc`, `_MemFree`, `_OpenFile`, `_FileRead`, `_FileClose` → nombres C sin `_` (replayer AHX).
- Avisos suprimidos solo en ELF donde el código NDK / optimización choca con **`-Werror`**.
- **`CPPFLAGS`**: `-D__stdargs=` para cabeceras del NDK Bartman.
- **Windows host**: `HOST_IS_WINDOWS`, **`PYTHONPATH`** sin sufijo roto para Python nativo; redirección de utilidades Go vía WSL (ver siguiente sección).

### Enlace, DWARF y efectos

- **`system/amiga-elf.lds`**: secciones **`.debug_*`**, `PROVIDE` de hardware (`_custom`, etc.) alineado con el convenio del proyecto en ELF.
- **`build/effect.mk`**: enlace ELF, **`elf2hunk`**, `Effect` con `--defsym`, strip con **`objcopy --strip-all`** si no hay `strip.exe`.
- **Makefiles de datos embebidos** (`objcopy -I binary`): ramas **`TOOLCHAIN_ELF`** con **`-O elf32-m68k`** y `redefine-sym` coherente con los `extern` en C (loader, playahx, playp61, playctr, textscroll, playpt, demo/playpt).

### Herramientas y código

- **`tools/elf-rename-vasm-syms.sh`**, **`tools/effect-tag.sh`** (extracción robusta del tag `EFFECT`).
- **Registros en C**: macro **`__ASM_REG_PARM`** (vacía en ELF; `asm("a0")` en `m68k-amigaos`).
- **Ajustes puntuales**: asm inline (`plasma`, `tiles8`), símbolos loader (`__ELF__` + `_LoaderModule`), warnings (`playpt` / packed), etc.

---

## Utilidades host en Go y wrapper WSL

En el árbol **`tools/`** hay varios proyectos Go (`go.mod`) usados en el build: por ejemplo **`png2c`**, **`obj2c`**, **`svg2c`**, **`tmxconv`**, **`pchg2c`**, **`sync2c`**, **`drng2c`**, **`packexe`**, **`splitexe`**, etc.

En **Linux** suelen ejecutarse como binarios nativos o compilados localmente. En **Windows (MSYS/MinGW)**, si no hay cadena Go nativa lista, **`build/common.mk`** redirige esas variables a:

```text
$(TOPDIR)/tools/wsl-go-tool.sh <subdir> …
```

**`tools/wsl-go-tool.sh`** hace:

1. Bajo WSL, **`go build -o <subdir>-linux`** en `tools/<subdir>/` si falta el binario.
2. Ejecuta ese ELF Linux con **`wsl`**, convirtiendo rutas con **`wslpath`**.

**El código fuente ya está en el repositorio** (no hace falta “buscar en internet” salvo que quieras actualizar dependencias o fork). Para **evitar la dependencia de WSL**, la alternativa directa es:

1. Instalar **[Go para Windows](https://go.dev/dl/)** (amd64).
2. En cada directorio bajo `tools/<nombre>/` que tenga `go.mod`, generar un `.exe`:

   ```bat
   cd tools\png2c
   set GOOS=windows
   set GOARCH=amd64
   go build -o png2c.exe .
   ```

3. En `common.mk` (o un fragmento incluido solo en Windows), apuntar `PNG2C`, `OBJ2C`, etc. a esos **`.exe`** en lugar de `wsl-go-tool.sh`.

Eso elimina WSL para el build; los mismos fuentes Go compilan para **Windows nativo** sin cambiar el diseño del demoscene. Si en el futuro se automatiza, conviene un script `tools/build-host-tools.ps1` / `.sh` que recorra los módulos y deje los binarios en una ruta fija.

---

## Python en Windows

Para reglas que usan Pillow u otras deps, a veces **Python de MSYS** no basta; el Makefile puede usar **`C:\Python3xx\python.exe`** con **`PYTHONPATH`** solo hacia `tools/pylib` (sin concatenar `:` que rompe el intérprete nativo).

---

## Pregunta frecuente: ¿“no soportan los mismos convenios de asm ni enlazado”?

**Enlazado:** el camino ELF usa **ELF + script GNU ld** y luego **Hunk**; el camino clásico usa **Hunk de punta a punta**. Los símbolos globales y las relocalizaciones no son intercambiables sin conversión (`objcopy`, `elf2hunk`, redefiniciones).

**Asm:**

- **C → gas:** sintaxis y prefijos de registro deben ser los que espera **GNU as** del GCC que emite el `.s`.
- **`.asm` → vasm:** en ELF, convención de exportación **`_Nombre`** frente a C **`Nombre`**, más includes NDK (`exec/types.i`) resueltos con **`-I`** al `sys-include` del Bartman.

No es que el 68000 sea otro; es que la **cadena de herramientas** (formato, linker, reglas de nombre) es distinta y el proyecto adapta el código y los Makefiles a ambas cadenas con **`#ifdef __ELF__`**, **`TOOLCHAIN_ELF`** y scripts auxiliares.

---

## Trabajo futuro y decisiones diferidas (sin cambios en código por ahora)

Esta subsección **no describe el estado del repo como tarea pendiente implementada**: solo deja constancia, por si más adelante se retoma el tema, de lo analizado y descartado **por ahora** sin tocar Makefiles ni herramientas.

### Utilidades host (Go, WSL, alternativas)

- **Herramientas realmente usadas en el árbol de build** (además de Python): sobre todo **`png2c`** y **`obj2c`** (muy frecuentes), más **`svg2c`**, **`tmxconv`**, **`pchg2c`**, **`drng2c`**, **`sync2c`**, **`packexe`**, **`splitexe`** según efectos y `demo/`.
- **`png2c` / `obj2c`** son el ecosistema **ghostown.pl** (código ya en `tools/*/`, no un binario mágico externo). Los flags (`--bitmap`, `--pixmap`, `--sprite`, etc.) son **específicos del proyecto**; no hay en la red un sustituto que haga *lo mismo* sin reimplementar el formato de salida.
- **Reescribir esas herramientas en C++** sería un proyecto grande (PNG, paletas, plantillas Amiga, OBJ→C, …); **no está planificado** salvo necesidad extrema.
- **Opciones si en el futuro se quiere evitar WSL** sin tocar la lógica de los conversores:
  1. **Go nativo Windows**: instalador oficial o, en MSYS2, paquete tipo **`mingw-w64-x86_64-go`**, luego `go build` en cada `tools/<módulo>/` y ajustar `PNG2C` / `OBJ2C` / … en `common.mk` (o script `tools/build-host-tools.*`) para usar los `.exe`.
  2. **Publicar o versionar `.exe`** (releases, artefacto de CI, opcionalmente Git LFS) para quien no quiera compilar host tools.
  3. **Mantener `tools/wsl-go-tool.sh`** si WSL sigue siendo aceptable.

### `objcopy` y `--redefine-sym` sobre binarios embebidos

- Con **`-I binary -O elf32-m68k`**, GNU `objcopy` genera símbolos con un **patrón fijo** derivado de la ruta del fichero de entrada (`_binary_…_start` / `_end` / `_size`). **No** se puede pedir otro nombre con un único flag tipo “nombre simbólico libre”.
- El **C del proyecto** declara `extern` con nombres históricos (p. ej. `binary_data_…`, `Text`, `Module`, `_LoaderModule`, …) alineados con el flujo Hunk o convenciones manuales.
- **`--redefine-sym antiguo=nuevo`** solo **renombra símbolos en el `.o`**; el contenido del blob es el mismo. **No** indica que ELF esté “mal”; es **pegamento de nombres** entre convención de `objcopy` y el código existente.
- **Si en el futuro se quisiera eliminar los `redefine-sym`**: habría que **cambiar todos los `extern` y usos** para coincidir exactamente con los `_binary_…` que produzca cada regla (sensible a la ruta pasada a `objcopy`), **o** concentrar alias en el script de enlace (`PROVIDE` / `--defsym`) — mismo concepto, otro sitio.

### Resumen

| Idea | Estado documentado |
|------|-------------------|
| Sustituir `png2c` por utilidad genérica de internet | No equivalente funcional sin rehacer el pipeline. |
| Port a C++ de las tools Go | Posible en teoría; coste muy alto; **no priorizado**. |
| Go nativo / MSYS2-go / exes precompilados | **Vía razonable** si se quiere dejar WSL; pendiente de decisión e implementación. |
| Quitar `redefine-sym` | Posible renombrando C o moviendo alias al linker; **refactor amplio**; pendiente. |

---

## Referencias en el repo

- Uso operativo: [TOOLCHAIN.md](TOOLCHAIN.md)
- Entorno: `activate`, `activate-barto`, `tools/with-demoscene-env.sh`, `.demoscene-env.mk` (generado)
