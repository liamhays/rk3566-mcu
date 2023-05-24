/*
*  Syntacore SCR* framework
*  @brief Bare metal tests/benchmarks linker script
*  @author mn-sc
*
* Copyright by Syntacore © 2017. ALL RIGHTS RESERVED.
*
*/
/* Adapted to the RK3566 implementation of the SCR1 core. */

OUTPUT_ARCH( "riscv" )
ENTRY(_start)

MEMORY {
  /* RK3566 SYSTEM_SRAM is the same size as SCR1 TCM */
  SYSTEM_SRAM (rwx) : ORIGIN = 0xFDCC0000, LENGTH = 64K
}

STACK_SIZE = 2048;

SECTIONS {

  .text.crt ORIGIN(SYSTEM_SRAM) : {
    *(.text.crt*)
  } >SYSTEM_SRAM

  .text : {
    PROVIDE(__TEXT_START__ = .);
    *(.text .text.*)
     PROVIDE(__TEXT_END__ = .);
  } >SYSTEM_SRAM

  .rodata : {
    _gp = . + 0x800;
    __global_pointer$ = . + 0x800;
    *(.srodata.cst16) *(.srodata.cst8) *(.srodata.cst4) *(.srodata.cst2) *(.srodata*)
  } >SYSTEM_SRAM

  /* data segment */
  .sdata : {
    PROVIDE(__DATA_START__ = .);
    *(.sdata .sdata.* .gnu.linkonce.s.*)
  } >SYSTEM_SRAM

  .data : {
    *(.data .data.*)
    . = ALIGN(4);
    PROVIDE(__DATA_END__ = .);
  } >SYSTEM_SRAM

  /* bss segment */
  .sbss : {
    PROVIDE(__BSS_START__ = .);
    *(.sbss .sbss.* .gnu.linkonce.sb.*)
    *(.scommon)
  } >SYSTEM_SRAM

  .bss : {
    *(.bss .bss.*)
    . = ALIGN(4);
    PROVIDE(__BSS_END__ = .);
  } >SYSTEM_SRAM

  . = ALIGN(16);

  _end = .;
  PROVIDE(__end = .);

  /* End of uninitalized data segement */

  .stack ORIGIN(SYSTEM_SRAM) + LENGTH(SYSTEM_SRAM) - STACK_SIZE : {
    PROVIDE(__STACK_START__ = .);
    . += STACK_SIZE;
    PROVIDE(__C_STACK_TOP__ = .);
    PROVIDE(__STACK_END__ = .);
  } >SYSTEM_SRAM

  /DISCARD/ : {
    *(.eh_frame .eh_frame.*)
  }
}
