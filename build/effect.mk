EFFECT := $(notdir $(CURDIR))

ifndef SOURCES
SOURCES := $(strip $(foreach f,$(EFFECT).c $(EFFECT).cpp,$(if $(wildcard $(f)),$(f),)))
ifeq ($(SOURCES),)
SOURCES = $(EFFECT).c
endif
endif

LOADABLES ?= $(EFFECT).exe
DEMO ?=

LIBS += libblit libgfx libmisc libc
LDEXTRA = $(TOPDIR)/system/system.a
LDEXTRA += $(foreach lib,$(LIBS),$(TOPDIR)/lib/$(lib)/$(lib).a)

CRT0 = $(TOPDIR)/system/crt0.o
MAIN ?= $(TOPDIR)/effects/main.o
BOOTLOADER = $(TOPDIR)/bootloader.bin
BOOTBLOCK = $(TOPDIR)/addchip.bootblock.bin
VBRMOVE = $(TOPDIR)/vbrmove

EXTRA-FILES += $(EFFECT).adf
CLEAN-FILES += $(LOADABLES)
CLEAN-FILES += $(EFFECT).exe $(EFFECT).exe.dbg $(EFFECT).exe.map $(EFFECT).log
ifdef TOOLCHAIN_ELF
CLEAN-FILES += $(EFFECT).elf $(EFFECT).st.elf
endif

all: build

# Check if library is up-to date if someone is asking explicitely
# «+» pasa el jobserver de make -j al sub-make (paraleliza varias libs a la vez).
$(TOPDIR)/lib/lib%.a: FORCE
	+$(MAKE) -C $(dir $@) $(notdir $@)

$(TOPDIR)/system/%.o: FORCE
	+$(MAKE) -C $(dir $@) $(notdir $@)

$(TOPDIR)/system/%.a: FORCE
	+$(MAKE) -C $(dir $@) $(notdir $@)

$(TOPDIR)/effects/%.a: FORCE
	+$(MAKE) -C $(dir $@) $(notdir $@)

$(TOPDIR)/effects/%.o: FORCE
	+$(MAKE) -C $(dir $@) $(notdir $@)

$(TOPDIR)/%.bin: FORCE
	+$(MAKE) -C $(dir $@) $(notdir $@)

include $(TOPDIR)/build/common.mk

ifeq ($(DEMO), 1)
CFLAGS := $(filter-out -msmall-code, $(CFLAGS))
endif

_EFFECT_SRC := $(firstword $(wildcard $(EFFECT).c $(EFFECT).cpp))
ifneq ($(_EFFECT_SRC),)
# Extracción en script: evita que Make malinterprete \(, comas o comillas dentro de $(shell …).
_EFFECT_TAG := $(shell sh $(TOPDIR)/tools/effect-tag.sh "$(abspath $(_EFFECT_SRC))")
ifneq ($(_EFFECT_TAG),)
ifdef TOOLCHAIN_ELF
# m68k-amiga-elf: main.c referencia el símbolo C «Effect»; EFFECT() define «EmptyEffect», etc.
LDFLAGS_EXTRA ?= -Wl,--defsym=Effect=$(_EFFECT_TAG)Effect
else
LDFLAGS_EXTRA ?= -defsym=_Effect=_$(_EFFECT_TAG)Effect
endif
endif
endif

ifdef TOOLCHAIN_ELF
$(EFFECT).elf: $(CRT0) $(MAIN) $(OBJECTS) $(LDEXTRA) $(LDSCRIPT)
	@echo "[LD] $(addprefix $(DIR),$(OBJECTS)) -> $(DIR)$@"
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDFLAGS_EXTRA) \
		-Wl,-T$(LDSCRIPT) -Wl,-Map=$(EFFECT).exe.map \
		-o $@ \
		-Wl,--start-group $(filter-out %.lds,$^) -Wl,--end-group

$(EFFECT).exe.dbg $(EFFECT).exe: $(EFFECT).elf
	@echo "[ELF2HUNK] $(DIR)$< -> $(DIR)$(EFFECT).exe"
	$(CP) $(EFFECT).elf $(EFFECT).exe.dbg
	$(OBJCOPY) --strip-all $< $(EFFECT).st.elf
	elf2hunk $(EFFECT).st.elf $(EFFECT).exe
	$(RM) $(EFFECT).st.elf
else
$(EFFECT).exe.dbg $(EFFECT).exe: $(CRT0) $(MAIN) $(OBJECTS) $(LDEXTRA) $(LDSCRIPT)
	@echo "[LD] $(addprefix $(DIR),$(OBJECTS)) -> $(DIR)$@"
	$(LD) $(LDFLAGS) $(LDFLAGS_EXTRA) -L$(TOPDIR)/system -T$(LDSCRIPT) -Map=$@.map -o $@ \
		--start-group $(filter-out %.lds,$^) --end-group
	$(CP) $@ $@.dbg
	$(STRIP) $@
endif

data/%.c: data/%.obj
	@echo "[MODEL/OBJ] $(DIR)$< -> $(DIR)$@"
	$(OBJ2C) $(OBJ2C.$*) $< $@

data/%.c: data/%.psfu
	@echo "[PSF] $(DIR)$^ -> $(DIR)$@"
	$(PSF2C) $(PSF2C.$*) $< > $@ || (rm -f $@ && exit 1)

data/%.c: data/%.png
	@echo "[PNG] $(DIR)$< -> $(DIR)$@"
	$(PNG2C) $(PNG2C.$*) $< > $@ || (rm -f $@ && exit 1)

data/%.c: data/%.svg
	@echo "[SVG] $(DIR)$< -> $(DIR)$@"
	$(SVG2C) $(SVG2C.$*) -o $@ $<

data/%.c: data/%.2d
	@echo "[2D] $(DIR)$< -> $(DIR)$@"
	$(CONV2D) $(CONV2D.$*) $< > $@ || (rm -f $@ && exit 1)

data/%.c: data/%.sync
	@echo "[SYNC] $(DIR)$< -> $(DIR)$@"
	$(SYNC2C) $(SYNC2C.$*) $< > $@ || (rm -f $@ && exit 1)

data/%.c: data/%.csv
	@echo "[ANIM2C] $(DIR)$< -> $(DIR)$@"
	$(ANIM2C) $(ANIM2C.$*) $< > $@ || (rm -f $@ && exit 1)

data/%.trk: data/%.mod
	@echo "[PTSPLIT] $(DIR)$< -> $(DIR)$@"
	$(PTSPLIT) --extract=trk $(PTSPLIT.$*) -o $@ $^

data/%.smp: data/%.mod
	@echo "[PTSPLIT] $(DIR)$< -> $(DIR)$@"
	$(PTSPLIT) --extract=smp $(PTSPLIT.$*) -o $@ $^

EXTRA-FILES += $(EFFECT).img
CLEAN-FILES += $(EFFECT).img

%.img: $(LOADABLES) $(DATA)
	@echo "[IMG] $(addprefix $(DIR),$<) -> $(DIR)$@"
	$(FSUTIL) create $@ $(filter-out %bootloader.bin,$^)

%.adf: %.img $(BOOTLOADER)
	@echo "[ADF] $(DIR)$< -> $(DIR)$@"
	$(ADFUTIL) -b $(BOOTLOADER) $< $@

%-dos.adf: %.exe $(BOOTBLOCK)
	@echo "[ADF] $(DIR)$< -> $(DIR)$@"
	echo $< > startup-sequence
	xdftool $@ format dos + write $< + makedir s + write startup-sequence s
	dd if=$(BOOTBLOCK) of=$@ conv=notrunc status=none
	rm startup-sequence

# Default debugger - can be changed by passing DEBUGGER=xyz to make.
DEBUGGER ?= gdb

run: $(EFFECT).exe.dbg $(EFFECT).adf
	$(LAUNCH) -e $(EFFECT).exe.dbg -f $(EFFECT).adf

debug: $(EFFECT).exe.dbg $(EFFECT).adf $(TOPDIR)/gdb-dashboard
	$(LAUNCH) -d $(DEBUGGER) -f $(EFFECT).adf -e $(EFFECT).exe.dbg

# Solo lanza WinUAE/uaedbg (sin make sobre libs/sistema). Tras «BUILD», usa esto para F5 rápido.
.PHONY: gdbserver-launch
gdbserver-launch:
	@test -f $(EFFECT).exe.dbg -a -f $(EFFECT).adf || { \
	  printf '%s\n' "gdbserver-launch: faltan $(EFFECT).exe.dbg o $(EFFECT).adf; ejecuta BUILD (o make) antes." >&2; \
	  exit 1; \
	}
	$(LAUNCH) -d gdbserver -f $(EFFECT).adf -e $(EFFECT).exe.dbg

$(TOPDIR)/gdb-dashboard:
	make -C $(TOPDIR) gdb-dashboard

.PHONY: run debug
.PRECIOUS: $(BOOTLOADER) $(BOOTBLOCK) $(EFFECT).img
