all:
	riscv32-unknown-linux-gnu-as read_store_mimpid.S -o read_store_mimpid.o
	riscv32-unknown-linux-gnu-ld -Tread_store_mimpid.ld read_store_mimpid.o -o read_store_mimpid
	riscv32-unknown-linux-gnu-objcopy -O binary read_store_mimpid read_store_mimpid.bin
