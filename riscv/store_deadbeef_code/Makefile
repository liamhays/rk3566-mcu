all:
	riscv32-unknown-linux-gnu-as store_deadbeef.S -o store_deadbeef.o
	riscv32-unknown-linux-gnu-ld -Tstore_deadbeef.ld store_deadbeef.o -o store_deadbeef.elf
	riscv32-unknown-linux-gnu-objcopy -O binary store_deadbeef.elf store_deadbeef.bin
	xxd -i store_deadbeef.bin > ../../kernel/read_deadbeef/riscv_code.h
