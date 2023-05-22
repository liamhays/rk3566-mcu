all:
	riscv32-unknown-linux-gnu-as -o ncrt0.o ncrt0.S
	riscv32-unknown-linux-gnu-gcc -c -nostdlib -nostartfiles -lgcc -o c_mailbox.o c_mailbox.c
	riscv32-unknown-linux-gnu-ld -T c_mailbox.ld -o c_mailbox.elf ncrt0.o c_mailbox.o
	riscv32-unknown-linux-gnu-objcopy -O binary c_mailbox.elf c_mailbox.bin
	xxd -i c_mailbox.bin > ../../kernel/read_c_mailbox/riscv_code.h
