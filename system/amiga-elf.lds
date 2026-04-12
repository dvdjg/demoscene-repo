/* ELF32 linker script for m68k-amiga-elf + elf2hunk (extensión BartmanAbyss).
 * Misma disposición de secciones que amiga.lds (chip / fast). */
OUTPUT_FORMAT("elf32-m68k")
OUTPUT_ARCH(m68k)
ENTRY(start)
PROVIDE(_SysBase = 0x4);
/* Hardware: el código C declara _ciaa / _ciab / _custom (una barra baja). */
PROVIDE(_ciaa = 0xbfe001);
PROVIDE(_ciab = 0xbfd000);
PROVIDE(_custom = 0xdff000);
SECTIONS
{
  . = 0x0;
  .text : {
    __text = .;
    *(.text.unlikely .text.unlikely.*)
    *(.text.startup .text.startup.*)
    *(.text)
    *(.text.*)
    *(.rodata)
    *(.rodata.*)
  }
  __text_size = SIZEOF(.text);
  . = ALIGN(8);
  .data : {
    __data = .;
    CONSTRUCTORS
    *(.data)
    *(.data.*)
  }
  __data_size = SIZEOF(.data);
  . = ALIGN(8);
  .bss : {
    __bss = .;
    *(.bss)
    *(.bss.*)
    *(COMMON)
  }
  __bss_size = SIZEOF(.bss);
  . = ALIGN(8);
  .datachip : {
    __data_chip = .;
    *(.datachip)
    *(.datachip.*)
  }
  __data_chip_size = SIZEOF(.datachip);
  . = ALIGN(8);
  .bsschip : {
    __bss_chip = .;
    *(.bsschip)
    *(.bsschip.*)
  }
  __bss_chip_size = SIZEOF(.bsschip);
  .stab 0 : { *(.stab) }
  .stabstr 0 : { *(.stabstr) }

  /* DWARF / metadatos GCC (-ggdb3); VMA 0 — no ocupan memoria del ejecutable en chip.
   * Sin esto, -Wl,--orphan-handling=error falla al enlazar .o con depuración. */
  .debug_aranges  0 : { *(.debug_aranges) }
  .debug_info      0 : { *(.debug_info .gnu.linkonce.wi.*) }
  .debug_abbrev    0 : { *(.debug_abbrev) }
  .debug_line      0 : { *(.debug_line .debug_line.* .debug_line_end) }
  .debug_frame     0 : { *(.debug_frame) }
  .debug_str       0 : { *(.debug_str) }
  .debug_loc       0 : { *(.debug_loc) }
  .debug_macinfo   0 : { *(.debug_macinfo) }
  .debug_rnglists  0 : { *(.debug_rnglists) }
  .debug_loclists  0 : { *(.debug_loclists) }
  .debug_macro     0 : { *(.debug_macro) }
  .debug_ranges    0 : { *(.debug_ranges) }
  .comment         0 : { *(.comment) }
}
