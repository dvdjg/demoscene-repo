TOPDIR := $(realpath ../..)

OBJECTS := data/jazzcat-sunglasses_at_night.o
CLEAN-FILES := data/drdos8x8.c

LIBS := libp61

PSF2C.drdos8x8 := --name drdos8x8 --type console

include $(TOPDIR)/build/effect.mk

OBJCOPY-CHIPMEM := --rename-section .data=.datachip,alloc,load,data,contents

ifdef TOOLCHAIN_ELF
OBJCOPY_BFD := elf32-m68k
endif

data/%.o: data/%.p61
	@echo "[OBJCOPY] $(DIR)$< -> $(DIR)$@"
ifdef TOOLCHAIN_ELF
	$(OBJCOPY) $(OBJCOPY-CHIPMEM) -I binary -O $(OBJCOPY_BFD) \
	  --redefine-sym _binary_data_$(subst -,_,$*)_p61_start=binary_data_$(subst -,_,$*)_p61_start \
	  --redefine-sym _binary_data_$(subst -,_,$*)_p61_end=binary_data_$(subst -,_,$*)_p61_end \
	  --redefine-sym _binary_data_$(subst -,_,$*)_p61_size=binary_data_$(subst -,_,$*)_p61_size \
	  $^ $@
else
	$(OBJCOPY) -I binary -O amiga $(OBJCOPY-CHIPMEM) $^ $@
endif
