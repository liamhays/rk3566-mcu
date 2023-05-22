OUTPUT_ARCH(riscv)
ENTRY(_start)

MEMORY
{
	NOT_ROM (rwx): ORIGIN = 0xFDCC0000, LENGTH = 32K
	RAM (rwx): ORIGIN = 0xFDCC8000, LENGTH = 32K
}

SECTIONS
{
	.text : {
      	      startup.o (.vector_table)
      	      *(.text)
      	      *(.rodata)
	} > NOT_ROM
	_text_end = .;
	.data : AT(ADDR(.text) + SIZEOF(.text))
	{
		_data_start = .;
		*(.data)
		. = ALIGN(8);
		_data_end = .;
	} > RAM
	.bss : {
     	     _bss_start = .;
     	     *(.bss)
     	     . = ALIGN(8);
     	     _bss_end = .;
	} > RAM
}

