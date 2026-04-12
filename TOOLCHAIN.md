# Toolchain Amiga (Bartman / m68k-amiga-elf) y C++

Este repositorio usa por defecto el toolchain de la extensión **Amiga C/C++ Compile, Debug & Profile** (BartmanAbyss): prefijo **`m68k-amiga-elf`**, enlace **ELF** y conversión a ejecutable Amiga con **`elf2hunk`**.

En Linux sigue siendo posible el paquete **demoscene-toolchain** (`m68k-amigaos-*`, salida Hunk directa); véase más abajo.

## Activación del entorno

- **Por defecto (recomendado con la extensión instalada):** las tareas de VS Code/Cursor y el script  
  `bash tools/with-demoscene-env.sh …` intentan primero `activate-barto`. Si la extensión no está (p. ej. CI Linux), se ejecuta `activate` (toolchain `.deb`).
- **Forzar solo el toolchain Linux (.deb):** crea en la raíz del repo un archivo vacío  
  **`.force-linux-demoscene-toolchain`**. Así se omite `activate-barto` y solo se usa `source activate` (Debian/Ubuntu + `/opt/amiga`).

Tras `source activate` o `source activate-barto`, Make lee **`.demoscene-env.mk`** (generado por esos scripts). En MSYS, es necesario para que GNU Make vea `DEMOSCENE_ROOT` y `M68K_TRIPLET`.

### Windows: dependencias Python para F5 / `make debug` / `make run`

`tools/launch.py` importa **`libtmux`** (está en `requirements.txt`). En Windows, **`build/common.mk`** elige el intérprete host en este orden: **`py -3`** si responde, si no **`py`**, si no **`C:\Python3xx\python.exe`** conocido; solo si nada de eso aplica se usa **`python3`** de MSYS (a menudo sin tus paquetes). Instala dependencias con el **mismo** launcher que quieras que use Make:

```bat
py -3 -m pip install -r requirements.txt
```

(o `py -m pip …` si no tienes `-3`). También vale `C:\Python313\python.exe -m pip install -r requirements.txt` si Make acaba usando esa ruta.

Sin eso, F5 puede fallar con `ModuleNotFoundError: No module named 'libtmux'`.

`launch.py` usa **tmux** si `tmux` está en el **PATH** del proceso (p. ej. Linux, o MSYS2 con `pacman -S tmux`). **Python nativo de Windows** no ve el `tmux` de Git Bash: si no hay `tmux`, para **`gdbserver`** y **`run`** se lanza **`uaedbg.py` directamente** (sin socat serial/parallel). El modo **`-d gdb`** (GDB en otra ventana tmux) sigue requiriendo tmux o depuración manual.

**Depuración GDB en Windows:** con el toolchain Bartman instalado desde la extensión, en **`bin/win32`** suele estar **`winuae-gdb.exe`** (WinUAE con gdbserver). Para **`make debug DEBUGGER=gdbserver`**, `launch.py` lo detecta (también vía **`WINUAE_GDB_EXE`**) y arranca WinUAE con un `.uae` temporal; GDB en VS Code usa **`localhost:2345`** (mismo puerto que la extensión Amiga Debug con WinUAE; gdbserver RSP nativo, no el stub Python de `uaedbg.py`). En Linux con FS-UAE, `uaedbg.py` también escucha en **2345** para unificar la configuración del IDE.

**FS-UAE en Windows (reserva):** si no hay `winuae-gdb.exe`, `uaedbg.py` intenta **`FS-UAE.exe`**. Si falla con *archivo no encontrado*, instala FS-UAE, añádelo al `PATH`, o define **`FS_UAE_EXE`** (o **`UAE_EMULATOR`**) con la ruta completa al `.exe`.

**F5 más rápido en VS Code/Cursor:** las tareas de build y `gdbserver` usan **`make -j8`** y **`--no-print-directory`**. Para no recorrer submakes en cada F5, usa la configuración **«Amiga GDB (sin rebuild)»** (tarea `gdbserver-only` → `make gdbserver-launch`) tras haber compilado con **BUILD**. El objetivo **`gdbserver-launch`** en `effect.mk` solo arranca el emulador si ya existen `$(EFFECT).exe.dbg` y `$(EFFECT).adf`.

## C++: estándar y modo “ligero”

- **Estándar por defecto:** **`gnu++23`** (`DEMOSCENE_CXXSTD`, sobrescribible en la línea de `make`).
- **Orientación a binario pequeño y rápido (freestanding):**
  - Sin excepciones: **`-fno-exceptions`**
  - Sin RTTI: **`-fno-rtti`**
  - Sin registro de destructores globales vía `__cxa_atexit`: **`-fno-use-cxa-atexit`**
  - Mismo nivel de optimización y flags de CPU que C (`OFLAGS` / `DEBUGBUILD`).
- **No** se enlaza **libstdc++** (`-nostdlib` en el camino ELF). Usa solo lo que el compilador inyecte (p. ej. **libgcc** para ayudas aritméticas) y el runtime propio del proyecto.

Los prototipos con `asm("a0")` en parámetros no son válidos en `m68k-amiga-elf`; el proyecto usa la macro **`__ASM_REG_PARM(...)`** en cabeceras (vacía en ELF, `asm(...)` en `m68k-amigaos`).

## Compilación optimizada (por defecto)

Objetivo: **`-O2`**, frame pointer omitido (`-fomit-frame-pointer`), sin flags de optimización agresivas extra (sin `-fstrict-aliasing` / desenrollado automático del perfil `-O2` por defecto de este Makefile).

```bash
# Desde la raíz, con entorno ya activo o vía wrapper:
bash tools/with-demoscene-env.sh make -C effects/empty

# O explícito:
make DEBUGBUILD=0 -C effects/empty
```

## Compilación de depuración

Objetivo: **`-O0`**, **`-fno-omit-frame-pointer`** para mejores trazas en GDB y variables locales más fiables.

```bash
make DEBUGBUILD=1 clean -C effects/mi_efecto
make DEBUGBUILD=1 -C effects/mi_efecto
```

En VS Code/Cursor la tarea **BUILD-DEBUG** hace `clean` + build con `DEBUGBUILD=1` para el efecto configurado en `settings.json` → `effect`.

**Depuración con GDB:** con el toolchain ELF, **`*.exe.dbg`** es una copia del **ELF** con símbolos; el depurador debe ser **`m68k-amiga-elf-gdb`**. Rutas típicas en `settings.json`: `demoscene.amigaElfGdb` / `demoscene.amigaElfGcc`.

## Variables útiles

| Variable            | Uso |
|---------------------|-----|
| `DEBUGBUILD=0/1`    | Optimizado vs depuración (C y C++). |
| `DEMOSCENE_CXXSTD` | Ej.: `gnu++23` (defecto), `gnu++20`, etc. |
| `M68K_TRIPLET`     | Fijada por `activate-barto` (`m68k-amiga-elf`) o `activate` (`m68k-amigaos`). |
| `MODEL=A500/A1200` | CPU 68000 vs 68020 (véase `activate` / `activate-barto`). |

## Enlace ELF (Bartman): detalles que afectan al build

- **Secciones de depuración:** con `-ggdb3` y `-Wl,--orphan-handling=error`, el enlazador exige que el script coloque las secciones DWARF (`.debug_*`, `.comment`, `.debug_ranges`). Están definidas en `system/amiga-elf.lds` con VMA 0 (metadatos del ELF, no memoria chip del Amiga).
- **Registros de hardware:** el código declara `_ciaa`, `_ciab` y `_custom` (una barra baja). En `amiga-elf.lds` se hace `PROVIDE` de esos nombres (el script `amiga.lds` de Hunk usaba `__ciaa` / `__custom`, distinto convención).
- **vasm + GCC:** con `-Felf`, `xdef _Nombre` exporta un símbolo con prefijo `_`, mientras el compilador C genera referencias a `Nombre`. Tras cada `vasm`, `tools/elf-rename-vasm-syms.sh` usa `objcopy --redefine-sym` sobre los globales `_X` → `X` (sin lista fija de `--defsym`, así no falla el enlazador si falta un `.o` concreto).
- **Símbolo `Effect`:** `effects/main.c` usa `extern EffectT Effect`; la macro `EFFECT(Foo,…)` define `FooEffect`. En Hunk se usa `-defsym=_Effect=_FooEffect`; en ELF, `-Wl,--defsym=Effect=FooEffect`. El nombre `Foo` se obtiene con `tools/effect-tag.sh` (evita que GNU Make rompa `$(shell …)` con `\(`, comas en `cut -d,` o rutas relativas al cwd del `shell`).
- **Strip en Windows:** el kit de la extensión no incluye `m68k-amiga-elf-strip.exe`. El camino ELF usa **`objcopy --strip-all`** para generar el ELF intermedio antes de `elf2hunk` (`build/effect.mk`).

## Archivos tocados por el soporte ELF / C++

- [TOOLCHAIN-ADAPTACION.md](TOOLCHAIN-ADAPTACION.md) — contexto de la adaptación Bartman, utilidades host (Go/WSL), `objcopy`/`redefine-sym`, y **decisiones diferidas** por si se retoma más adelante (sin obligar a cambios ahora).
- `build/common.mk` — triplete, `CXXFLAGS`, `DEMOSCENE_CXXSTD`, avisos, `RANLIB`, `OBJDUMP`, reglas `.cpp`, post-proceso vasm ELF.
- `build/effect.mk` — enlace ELF + `elf2hunk`, `Effect`/`LDFLAGS_EXTRA`, fuentes `.cpp`, `objcopy --strip-all`.
- `system/amiga-elf.lds` — script de enlace ELF32, DWARF, `PROVIDE` hardware.
- `tools/elf-rename-vasm-syms.sh`, `tools/effect-tag.sh`, `activate-barto`, `tools/with-demoscene-env.sh`, `.demoscene-env.mk` (generado, ignorado por git).
- Cabeceras con `__ASM_REG_PARM`, rutinas `.S` en libc con prólogo de pila bajo `__ELF__`, etc.

## Efecto de prueba C++

El efecto **`effects/empty`** incluye **`cxxsmoke.cpp`**, una unidad mínima C++23 (atributo `[[assume]]`) enlazada con el resto del efecto, para validar el pipeline C++ sin añadir dependencias pesadas.
