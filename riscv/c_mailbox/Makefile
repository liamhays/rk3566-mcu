OBJCOPY=riscv32-unknown-linux-gnu-objcopy
LD=riscv32-unknown-linux-gnu-ld
CC=riscv32-unknown-linux-gnu-gcc
AS=riscv32-unknown-linux-gnu-as

# SCR1 uses '-static -march=rv32im -mabi=ilp32 -std=gnu99
# -mstrict-align -msmall-data-limit=8 -ffunction-sections
# -fdata-sections -fno-common' as well as -mdiv if possible

# SCR1 in RK3566 is RV32IMC
CFLAGS=-static -nostdlib -mstrict-align -fno-common -Wall -nostartfiles -lgcc
LFLAGS=

all: c_mailbox.bin

ncrt0.o: ncrt0.S
	$(AS) -o ncrt0.o ncrt0.S

c_mailbox.o: c_mailbox.c
	$(CC) -c -o c_mailbox.o $(CFLAGS) c_mailbox.c

c_mailbox.elf: ncrt0.o c_mailbox.o
	$(LD) $(LFLAGS) -T c_mailbox.ld ncrt0.o c_mailbox.o -o c_mailbox.elf

c_mailbox.bin: c_mailbox.elf
	$(OBJCOPY) -O binary c_mailbox.elf c_mailbox.bin
	xxd -i c_mailbox.bin > ../../kernel/read_c_mailbox/riscv_code.h
