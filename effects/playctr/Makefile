TOPDIR := $(realpath ../..)

OBJECTS := data/JazzCat-Automatic-ctr.o data/JazzCat-Automatic-smp.o
CLEAN-FILES := data/lat2-08.c

LIBS := libctr

PSF2C.lat2-08 := --name latin2 --type console

include $(TOPDIR)/build/effect.mk

ifdef TOOLCHAIN_ELF
OBJCOPY_BFD := elf32-m68k
endif

data/%.ctr data/%.smp: data/%.mod
	$(PYTHON3) data/CinterConvert.py $^ $@ $(@:.ctr=.smp)

data/%-ctr.o: data/%.ctr
	@echo "[OBJCOPY] $(DIR)$< -> $(DIR)$@"
ifdef TOOLCHAIN_ELF
	$(OBJCOPY) -I binary -O $(OBJCOPY_BFD) \
	  --redefine-sym _binary_data_$(subst -,_,$*)_ctr_start=binary_data_$(subst -,_,$*)_ctr_start \
	  --redefine-sym _binary_data_$(subst -,_,$*)_ctr_end=binary_data_$(subst -,_,$*)_ctr_end \
	  --redefine-sym _binary_data_$(subst -,_,$*)_ctr_size=binary_data_$(subst -,_,$*)_ctr_size \
	  $^ $@
else
	$(OBJCOPY) -I binary -O amiga $^ $@
endif

data/%-smp.o: data/%.smp
	@echo "[OBJCOPY] $(DIR)$< -> $(DIR)$@"
ifdef TOOLCHAIN_ELF
	$(OBJCOPY) -I binary -O $(OBJCOPY_BFD) \
	  --redefine-sym _binary_data_$(subst -,_,$*)_smp_start=binary_data_$(subst -,_,$*)_smp_start \
	  --redefine-sym _binary_data_$(subst -,_,$*)_smp_end=binary_data_$(subst -,_,$*)_smp_end \
	  --redefine-sym _binary_data_$(subst -,_,$*)_smp_size=binary_data_$(subst -,_,$*)_smp_size \
	  $^ $@
else
	$(OBJCOPY) -I binary -O amiga $^ $@
endif
