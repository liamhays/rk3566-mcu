OBJCOPY=riscv-none-elf-objcopy
CC=riscv-none-elf-gcc
LD=$(CC)
AS=riscv-none-elf-as
# SCR1 uses '-static -march=rv32im -mabi=ilp32 -std=gnu99
# -mstrict-align -msmall-data-limit=8 -ffunction-sections
# -fdata-sections -fno-common' as well as -mdiv if possible

ASFLAGS=-march=rv32imc -mabi=ilp32 -misa-spec=2.2

# SCR1 in RK3566 is RV32IMC
CFLAGS=-static -march=rv32imc -mabi=ilp32 -nostdlib -mstrict-align -fno-common -Wall -nostartfiles -lgcc
# linker will default to rv64gc/lp64d if you don't specify the exact arch and ABI
# `-melf32lriscv` is from https://reddit.com/r/RISCV/comments/t2vl7e/gcc_linker_for_rv32i/

# it's apparently common knowledge to use gcc as the linker
LFLAGS=-nostartfiles -nostdlib -march=rv32imc -mabi=ilp32

PROJECT_NAME=hello_mailbox_rv

all: build/$(PROJECT_NAME).bin

.PHONY: clean

clean:
	rm -rf build/
# create build/ directory if it does not exist
build:
	mkdir $@

build/ncrt0.o: build
	$(AS) $(ASFLAGS) -o build/ncrt0.o ../../common/ncrt0.S

build/$(PROJECT_NAME).o: $(PROJECT_NAME).c build
	$(CC) -c -o build/$(PROJECT_NAME).o $(CFLAGS) $(PROJECT_NAME).c

build/$(PROJECT_NAME).elf: build build/ncrt0.o build/$(PROJECT_NAME).o
	$(LD) $(LFLAGS) -T ../../common/common.ld build/ncrt0.o build/$(PROJECT_NAME).o -o build/$(PROJECT_NAME).elf

# xxd is pretty dim and makes the variable name in the header
# file `build_$(PROJECT_NAME)_bin`, we don't want the `build`.
build/$(PROJECT_NAME).bin: build build/$(PROJECT_NAME).elf
	$(OBJCOPY) -O binary build/$(PROJECT_NAME).elf build/$(PROJECT_NAME).bin
	xxd -i build/$(PROJECT_NAME).bin > build/$(PROJECT_NAME)_bin.h
	sed -i 's/build_//g' build/$(PROJECT_NAME)_bin.h
