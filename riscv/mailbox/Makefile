all:
	riscv32-unknown-linux-gnu-as mailbox.S -o mailbox.o
	riscv32-unknown-linux-gnu-ld -Tmailbox.ld mailbox.o -o mailbox.elf
	riscv32-unknown-linux-gnu-objcopy -O binary mailbox.elf mailbox.bin
	xxd -i mailbox.bin > ../../kernel/mailbox/riscv_code.h
