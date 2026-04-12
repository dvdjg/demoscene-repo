TOPDIR := $(realpath ../..)

DATA := data/ahxWaves data/ahxWaves.Location
OBJECTS := data/jazzcat-electric_city.o
CLEAN-FILES := data/lat2-08.c

LIBS := libahx

PSF2C.lat2-08 := --name latin2 --type console

include $(TOPDIR)/build/effect.mk

ifdef TOOLCHAIN_ELF
OBJCOPY_BFD := elf32-m68k
endif

data/%.o: data/%.ahx
	@echo "[OBJCOPY] $(DIR)$< -> $(DIR)$@"
ifdef TOOLCHAIN_ELF
	$(OBJCOPY) -I binary -O $(OBJCOPY_BFD) \
	  --redefine-sym _binary_data_$(subst -,_,$*)_ahx_start=binary_data_$(subst -,_,$*)_ahx_start \
	  --redefine-sym _binary_data_$(subst -,_,$*)_ahx_end=binary_data_$(subst -,_,$*)_ahx_end \
	  --redefine-sym _binary_data_$(subst -,_,$*)_ahx_size=binary_data_$(subst -,_,$*)_ahx_size \
	  $^ $@
else
	$(OBJCOPY) -I binary -O amiga $^ $@
endif
