TOPDIR := $(realpath ../..)

MODULE := JazzCat-XenoCoq
OBJECTS := data/$(MODULE).trk.o data/$(MODULE).smp.o

DELTA := 1

ifeq ($(DELTA),1)
CPPFLAGS += -DDELTA
PTSPLIT.$(MODULE) := --delta
endif

LIBS := libpt

include $(TOPDIR)/build/effect.mk

CLEAN-FILES += data/$(MODULE)*.o data/$(MODULE).smp data/$(MODULE).trk

OBJCOPY-CHIP := --rename-section .data=.datachip,alloc,load,data,contents

ifdef TOOLCHAIN_ELF
OBJCOPY_BFD := elf32-m68k
endif

data/%.trk.o: data/%.trk
	@echo "[OBJCOPY] $(DIR)$< -> $(DIR)$@"
ifdef TOOLCHAIN_ELF
	$(OBJCOPY) -I binary -O $(OBJCOPY_BFD) \
	  --redefine-sym _binary_data_$(subst -,_,$*)_trk_start=Module \
	  --redefine-sym _binary_data_$(subst -,_,$*)_trk_end=ModuleEnd \
	  --redefine-sym _binary_data_$(subst -,_,$*)_trk_size=ModuleSize \
	  $^ $@
else
	$(OBJCOPY) -I binary -O amiga \
	  --redefine-sym _binary_data_$(subst -,_,$*)_trk_start=_Module \
	  --redefine-sym _binary_data_$(subst -,_,$*)_trk_end=_ModuleEnd \
	  --redefine-sym _binary_data_$(subst -,_,$*)_trk_size=_ModuleSize \
	  $^ $@
endif

data/%.smp.o: data/%.smp
	@echo "[OBJCOPY] $(DIR)$< -> $(DIR)$@"
ifdef TOOLCHAIN_ELF
	$(OBJCOPY) $(OBJCOPY-CHIP) -I binary -O $(OBJCOPY_BFD) \
	  --redefine-sym _binary_data_$(subst -,_,$*)_smp_start=Samples \
	  --redefine-sym _binary_data_$(subst -,_,$*)_smp_end=SamplesEnd \
	  --redefine-sym _binary_data_$(subst -,_,$*)_smp_size=SamplesSize \
	  $^ $@
else
	$(OBJCOPY) $(OBJCOPY-CHIP) -I binary -O amiga \
	  --redefine-sym _binary_data_$(subst -,_,$*)_smp_start=_Samples \
	  --redefine-sym _binary_data_$(subst -,_,$*)_smp_end=_SamplesEnd \
	  --redefine-sym _binary_data_$(subst -,_,$*)_smp_size=_SamplesSize \
	  $^ $@
endif
