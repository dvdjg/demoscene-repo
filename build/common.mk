export TOPDIR

MAKEFLAGS += --no-builtin-rules

# Algunos GNU Make (p. ej. MSYS) no importan variables del entorno del proceso;
# activate / activate-barto generan este fragmento en la raíz del repo.
ifdef TOPDIR
-include $(TOPDIR)/.demoscene-env.mk
endif

ifndef DEMOSCENE_ROOT
$(error Entorno no configurado: ejecuta "bash tools/with-demoscene-env.sh …" o "source activate-barto" / "source activate". Ver TOOLCHAIN.md)
endif

DIR := $(patsubst $(TOPDIR)%,%,$(realpath $(CURDIR)))
DIR := $(patsubst /%,%/,$(DIR))

# m68k-amigaos: demoscene-toolchain (.deb). m68k-amiga-elf: extensión BartmanAbyss + elf2hunk.
# activate-barto exporta M68K_TRIPLET=m68k-amiga-elf y amplía PATH.
M68K_TRIPLET ?= m68k-amigaos
ifeq ($(M68K_TRIPLET),m68k-amiga-elf)
TOOLCHAIN_ELF := 1
endif

# Compiler tools & flags definitions
ifdef TOOLCHAIN_ELF
CC	:= $(M68K_TRIPLET)-gcc
CXX	:= $(CC)
CPP	:= $(CC)
LD	:= $(CC)
AR	:= $(M68K_TRIPLET)-ar
# La extensión Bartman no incluye ranlib.exe; "ar s" actualiza el índice del .a
RANLIB	:= $(AR) s
VASM	:= vasmm68k_mot
else
CC	:= $(M68K_TRIPLET)-gcc
CXX	:= $(CC)
CPP	:= $(M68K_TRIPLET)-cpp
LD	:= $(M68K_TRIPLET)-ld
AR	:= $(M68K_TRIPLET)-ar
RANLIB	:= $(M68K_TRIPLET)-ranlib
VASM	:= vasm -quiet
endif

ifeq ($(MODEL),A1200)
CPU_AS = 68020
CPU_CC = 68020
else
CPU_AS = 68010
CPU_CC = 68000
endif

ASFLAGS	:= -m$(CPU_AS) -Wa,--register-prefix-optional -Wa,--bitwise-or
ifdef TOOLCHAIN_ELF
# -gdwarf en algunos .S (p. ej. inflate.S) provoca "unaligned opcodes" con gas 68000.
ASFLAGS	+= -Wa,-gstabs+
else
ASFLAGS	+= -Wa,-gstabs+
endif

ifdef TOOLCHAIN_ELF
# NDK ensamblador (exec/types.i, …): sys-include junto al prefijo del gcc cruzado (Bartman).
M68K_SYS_INCLUDE := $(abspath $(dir $(shell command -v $(CC) 2>/dev/null))../m68k-amiga-elf/sys-include)
VASMFLAGS	:= -m$(CPU_AS) -quiet -I$(TOPDIR)/include -Felf -dwarf=3
ifneq ($(wildcard $(M68K_SYS_INCLUDE)/exec/types.i),)
VASMFLAGS	+= -I$(M68K_SYS_INCLUDE)
endif
LDFLAGS	:= -nostdlib -Wl,--emit-relocs -Wl,--gc-sections -Wl,--orphan-handling=error
# libpt/pt.asm referencia _ExcVecBase; el símbolo C en ELF es ExcVecBase.
LDFLAGS	+= -Wl,--defsym=_ExcVecBase=ExcVecBase
# libahx/ahx.asm (jumptable AHX) referencia _MemAlloc…; en ELF los símbolos C no llevan _.
LDFLAGS	+= -Wl,--defsym=_MemAlloc=MemAlloc -Wl,--defsym=_MemFree=MemFree \
	-Wl,--defsym=_OpenFile=OpenFile -Wl,--defsym=_FileRead=FileRead \
	-Wl,--defsym=_FileClose=FileClose
LDSCRIPT	?= $(TOPDIR)/system/amiga-elf.lds
else
VASMFLAGS	:= -m$(CPU_AS) -quiet -I$(TOPDIR)/include
LDFLAGS	:= -amiga-debug-hunk --orphan-handling=error
LDSCRIPT	?= $(TOPDIR)/system/amiga.lds
endif

WFLAGS_COMMON := -Wall -W -Werror -Wundef -Wsign-compare -Wredundant-decls -Wwrite-strings
WFLAGS_C_ONLY := -Wnested-externs -Wstrict-prototypes
ifdef TOOLCHAIN_ELF
# Cabeceras NDK / proto con variables register + -O2 (toolchain Bartman).
WFLAGS_COMMON += -Wno-volatile-register-var -Wno-cast-function-type \
	-Wno-unused-but-set-variable -Wno-implicit-fallthrough \
	-Wno-pointer-sign -Wno-strict-aliasing \
	-Wno-aggressive-loop-optimizations -Wno-array-bounds
endif
CFLAGS	= -ggdb3 -ffreestanding -fno-common $(OFLAGS) $(WFLAGS_COMMON) $(WFLAGS_C_ONLY)
# C++ freestanding ligero: sin excepciones, RTTI, ni registro atexit de estáticas;
# -fno-threadsafe-statics evita bloqueos/código extra en inicialización de static locales.
CXXFLAGS = -ggdb3 -ffreestanding -fno-common $(OFLAGS) $(WFLAGS_COMMON) \
	-fno-exceptions -fno-rtti -fno-use-cxa-atexit -fno-threadsafe-statics \
	-std=$(DEMOSCENE_CXXSTD)
DEMOSCENE_CXXSTD ?= gnu++23
OFLAGS	:= -m$(CPU_CC)
ifndef TOOLCHAIN_ELF
# Opciones específicas del backend AmigaOS / demoscene-toolchain (no válidas en m68k-amiga-elf).
OFLAGS	+= -mregparm=2 -freg-struct-return
endif
# The '-O2' option does not turn on optimizations '-funroll-loops',
# '-funroll-all-loops' and `-fstrict-aliasing'.
# DEBUGBUILD=1: mejor visibilidad de variables locales en GDB (-O0, frame pointer).
DEBUGBUILD ?= 0
export DEBUGBUILD
ifeq ($(DEBUGBUILD),1)
OFLAGS	+= -O0 -fno-omit-frame-pointer
else
# -O2: no -funroll-loops / -funroll-all-loops / -fstrict-aliasing por defecto.
OFLAGS	+= -O2 -fomit-frame-pointer -fstrength-reduce
endif
CPPFLAGS += -I$(TOPDIR)/include

# Don't reload library base for each call
CPPFLAGS += -D__CONSTLIBBASEDECL__=const

# Cabeceras NDK del toolchain Bartman usan __stdargs (vacío en este entorno).
ifdef TOOLCHAIN_ELF
CPPFLAGS += -D__stdargs=
endif


# Pass "VERBOSE=1" at command line to display command being invoked by GNU Make
VERBOSE ?= 0

ifeq ($(VERBOSE), 0)
.SILENT:
QUIET := --quiet
endif

# Common tools definition
CP := cp -a
RM := rm -v -f
PYTHON3 := PYTHONPATH="$(TOPDIR)/tools/pylib:$$PYTHONPATH" python3
ADFUTIL := $(TOPDIR)/tools/adfutil.py
FSUTIL := $(TOPDIR)/tools/fsutil.py
BINPATCH := $(TOPDIR)/tools/binpatch.py
LAUNCH := $(PYTHON3) $(TOPDIR)/tools/launch.py
ANIM2C := $(TOPDIR)/tools/anim2c.py
CONV2D := $(TOPDIR)/tools/conv2d.py
DRNG2C := $(TOPDIR)/tools/drng2c/drng2c
GRADIENT := $(TOPDIR)/tools/gradient.py
TMXCONV := $(TOPDIR)/tools/tmxconv/tmxconv
PCHG2C := $(TOPDIR)/tools/pchg2c/pchg2c
PNG2C := $(TOPDIR)/tools/png2c/png2c
PSF2C := $(TOPDIR)/tools/psf2c.py
PTSPLIT := $(PYTHON3) $(TOPDIR)/tools/ptsplit.py
SYNC2C := $(TOPDIR)/tools/sync2c/sync2c
SVG2C := $(TOPDIR)/tools/svg2c/svg2c
PACKEXE := $(TOPDIR)/tools/packexe/packexe
SPLITEXE := $(TOPDIR)/tools/splitexe/splitexe
OBJ2C := $(TOPDIR)/tools/obj2c/obj2c

# Binarios Go en el repo son ELF Linux x86-64. En Windows (MSYS/MinGW) sin Go nativo,
# compilar y ejecutar vía WSL: tools/wsl-go-tool.sh <subdir> …
# (GNU Make en MSYS a veces no define OS=Windows_NT; usar también uname.)
HOST_IS_WINDOWS :=
ifeq ($(OS),Windows_NT)
HOST_IS_WINDOWS := 1
endif
ifneq (,$(findstring MINGW,$(shell uname -s 2>/dev/null)))
HOST_IS_WINDOWS := 1
endif
ifneq (,$(findstring MSYS,$(shell uname -s 2>/dev/null)))
HOST_IS_WINDOWS := 1
endif
ifeq ($(HOST_IS_WINDOWS),1)
WSL_AVAILABLE := $(shell wsl -e true >/dev/null 2>&1 && echo 1)
ifeq ($(WSL_AVAILABLE),1)
WSL_GO_TOOL := $(TOPDIR)/tools/wsl-go-tool.sh
DRNG2C := $(WSL_GO_TOOL) drng2c
TMXCONV := $(WSL_GO_TOOL) tmxconv
PCHG2C := $(WSL_GO_TOOL) pchg2c
PNG2C := $(WSL_GO_TOOL) png2c
SYNC2C := $(WSL_GO_TOOL) sync2c
SVG2C := $(WSL_GO_TOOL) svg2c
PACKEXE := $(WSL_GO_TOOL) packexe
SPLITEXE := $(WSL_GO_TOOL) splitexe
OBJ2C := $(WSL_GO_TOOL) obj2c
endif
# Python host en Windows: mismo intérprete que "py -m pip install -r requirements.txt".
# Orden: 1) launcher "py -3" / "py", 2) python.exe en rutas típicas, 3) se queda python3 (MSYS).
WINDOWS_PY_HOST :=
ifeq ($(shell command -v py >/dev/null 2>&1 && py -3 -c "import sys" 2>/dev/null && echo 1),1)
WINDOWS_PY_HOST := py -3
endif
ifeq ($(WINDOWS_PY_HOST),)
ifeq ($(shell command -v py >/dev/null 2>&1 && py -c "import sys" 2>/dev/null && echo 1),1)
WINDOWS_PY_HOST := py
endif
endif
ifneq ($(WINDOWS_PY_HOST),)
# Python nativo: un solo path en PYTHONPATH; no añadir ":$$PYTHONPATH" (':' final rompe imports).
PYTHON3 := PYTHONPATH="$(TOPDIR)/tools/pylib" $(WINDOWS_PY_HOST)
LAUNCH := $(PYTHON3) $(TOPDIR)/tools/launch.py
PTSPLIT := $(PYTHON3) $(TOPDIR)/tools/ptsplit.py
else
PYTHON_WIN := $(firstword $(wildcard /c/Python313/python.exe /c/Python312/python.exe /c/Python311/python.exe))
ifneq ($(PYTHON_WIN),)
PYTHON3 := PYTHONPATH="$(TOPDIR)/tools/pylib" $(PYTHON_WIN)
LAUNCH := $(PYTHON3) $(TOPDIR)/tools/launch.py
PTSPLIT := $(PYTHON3) $(TOPDIR)/tools/ptsplit.py
endif
endif
endif

STRIP := $(M68K_TRIPLET)-strip -s
OBJCOPY := $(M68K_TRIPLET)-objcopy
OBJDUMP := $(M68K_TRIPLET)-objdump

# Generate dependencies automatically
SOURCES_C = $(filter %.c,$(SOURCES))
SOURCES_CPP = $(filter %.cpp,$(SOURCES))
SOURCES_S = $(filter %.S,$(SOURCES))
SOURCES_ASM = $(filter %.asm,$(SOURCES))
OBJECTS += $(filter-out $(LOADABLES),\
             $(SOURCES_C:%.c=%.o) $(SOURCES_CPP:%.cpp=%.o) $(SOURCES_S:%.S=%.o) $(SOURCES_ASM:%.asm=%.o))

DEPENDENCY-FILES += $(foreach f, $(SOURCES_C),\
		      $(dir $(f))$(patsubst %.c,.%.D,$(notdir $(f))))
DEPENDENCY-FILES += $(foreach f, $(SOURCES_CPP),\
		      $(dir $(f))$(patsubst %.cpp,.%.D,$(notdir $(f))))
DEPENDENCY-FILES += $(foreach f, $(SOURCES_S),\
		      $(dir $(f))$(patsubst %.S,.%.D,$(notdir $(f))))

$(DEPENDENCY-FILES): $(SOURCES_GEN)

CLEAN-FILES += $(DEPENDENCY-FILES) $(SOURCES_GEN) $(OBJECTS) $(LOADABLES)
CLEAN-FILES += $(SOURCES:%=%~)

# Disable all built-in recipes and define our own
.SUFFIXES:

.%.D: %.c
	@echo "[DEP] $(DIR)$@"
ifdef TOOLCHAIN_ELF
	$(CC) $(CFLAGS) $(CFLAGS.$*) $(CPPFLAGS) $(CPPFLAGS.$*) -MM -MG $< | \
                sed -e 's,$(notdir $*).o,$*.o,g' > $@
else
	$(CPP) $(CPPFLAGS) $(CPPFLAGS.$*) -M -MG $< | \
                sed -e 's,$(notdir $*).o,$*.o,g' > $@
endif

.%.D: %.cpp
	@echo "[DEP] $(DIR)$@"
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(CPPFLAGS.$*) -M -MG $< | \
                sed -e 's,$(notdir $*).o,$*.o,g' > $@

.%.D: %.S
	@echo "[DEP] $(DIR)$@"
ifdef TOOLCHAIN_ELF
	$(CC) -MM -MG $(ASFLAGS) $(ASFLAGS.$*) $(CPPFLAGS) $(CPPFLAGS.$*) $< | \
                sed -e 's,$(notdir $*).o,$*.o,g' > $@
else
	$(CPP) $(CPPFLAGS) $(CPPFLAGS.$*) -M -MG $< | \
                sed -e 's,$(notdir $*).o,$*.o,g' > $@
endif

%.o: %.c
	@echo "[CC] $(DIR)$< -> $(DIR)$@"
	$(CC) $(CFLAGS) $(CFLAGS.$*) $(CPPFLAGS) $(CPPFLAGS.$*) \
                -c -o $@ $(abspath $<)

%.o: %.cpp
	@echo "[CXX] $(DIR)$< -> $(DIR)$@"
	$(CXX) $(CXXFLAGS) $(CXXFLAGS.$*) $(CPPFLAGS) $(CPPFLAGS.$*) \
                -c -o $@ $(abspath $<)

%.o: %.S
	@echo "[AS] $(DIR)$< -> $(DIR)$@"
	$(CC) $(ASFLAGS) $(ASFLAGS.$*) $(CPPFLAGS) $(CPPFLAGS.$*) \
                -c -o $@ $(abspath $<)

%.o: %.asm
	@echo "[VASM] $(DIR)$< -> $(DIR)$@"
ifdef TOOLCHAIN_ELF
	$(VASM) $(VASMFLAGS) $(VASMFLAGS.$*) -o $@ $<
	@bash $(TOPDIR)/tools/elf-rename-vasm-syms.sh $(OBJCOPY) $(OBJDUMP) $@
else
	$(VASM) -Fhunk $(VASMFLAGS) $(VASMFLAGS.$*) -o $@ $<
endif

%: %.asm
	@echo "[VASM] $(DIR)$< -> $(DIR)$@"
	$(VASM) -Fhunkexe -nosym $(VASMFLAGS) $(VASMFLAGS.$*) -o $@ $<

%.bin: %.asm
	@echo "[VASM] $(DIR)$< -> $(DIR)$@"
	$(VASM) -Fbin $(VASMFLAGS) -o $@ $<

%.S: %.c
	@echo "[CC] $(DIR)$< -> $(DIR)$@"
	$(CC) $(CFLAGS) $(CFLAGS.$*) $(CPPFLAGS) $(CPPFLAGS.$*) -fverbose-asm -S -o $@ $<

ifeq ($(words $(findstring $(MAKECMDGOALS), clean)), 0)
  -include $(DEPENDENCY-FILES)
endif

# Rules for recursive build
build-%: FORCE
	@echo "[MAKE] build $(DIR)$*"
	$(MAKE) -C $*

clean-%: FORCE
	@echo "[MAKE] clean $(DIR)$*"
	$(MAKE) -C $* clean

# Rules for build
subdirs: $(foreach dir,$(SUBDIRS),build-$(dir))

build: subdirs $(OBJECTS) $(LOADABLES) $(BUILD-FILES) $(EXTRA-FILES) 

clean: $(foreach dir,$(SUBDIRS),clean-$(dir)) 
	$(RM) $(BUILD-FILES) $(EXTRA-FILES) $(CLEAN-FILES) *~ *.taghl

.PHONY: all build subdirs clean FORCE

# vim: ts=8 et
