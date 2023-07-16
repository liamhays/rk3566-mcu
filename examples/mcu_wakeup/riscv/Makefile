OBJCOPY=riscv-none-elf-objcopy
CC=riscv-none-elf-gcc
LD=$(CC)
AS=riscv-none-elf-as
# SCR1 uses '-static -march=rv32im -mabi=ilp32 -std=gnu99
# -mstrict-align -msmall-data-limit=8 -ffunction-sections
# -fdata-sections -fno-common' as well as -mdiv if possible

# -misa-spec=2.2 is needed to get Zicsr extension
ASFLAGS=-march=rv32imc -mabi=ilp32 -misa-spec=2.2

# SCR1 in RK3566 is RV32IMC
CFLAGS=-static -march=rv32imc -mabi=ilp32 -misa-spec=2.2 -nostdlib -mstrict-align -fno-common -Wall -nostartfiles -lgcc
# linker will default to rv64gc/lp64d if you don't specify the exact arch and ABI
# `-melf32lriscv` is from https://reddit.com/r/RISCV/comments/t2vl7e/gcc_linker_for_rv32i/

# it's apparently common knowledge to use gcc as the linker
LFLAGS=-nostartfiles -nostdlib -march=rv32imc -mabi=ilp32 -misa-spec=2.2

PROJECT_NAME=mcu_wakeup_rv

# The pipe defines an order-only prerequisite, which prevents make from
# rebuilding all the code unless something changes. See
# https://www.twoistoomany.com/blog/2020/07/23/make-order-only-prerequisites/
all: build/$(PROJECT_NAME)_bin.h | build

.PHONY: clean

clean:
	rm -rf build/
# create build/ directory if it does not exist
build:
	mkdir $@

build/ncrt0.o: ../../common/ncrt0.S | build
	$(AS) $(ASFLAGS) -o build/ncrt0.o ../../common/ncrt0.S

build/$(PROJECT_NAME).o: $(PROJECT_NAME).c | build
	$(CC) -c -o $@ $(CFLAGS) $(PROJECT_NAME).c

build/$(PROJECT_NAME).elf: build/ncrt0.o build/$(PROJECT_NAME).o | build
	$(LD) $(LFLAGS) -T ../../common/common.ld build/ncrt0.o build/$(PROJECT_NAME).o -o $@

# xxd is pretty dim and makes the variable name in the header
# file `build_$(PROJECT_NAME)_bin`, we don't want the `build`.
build/$(PROJECT_NAME).bin: build/$(PROJECT_NAME).elf | build
	$(OBJCOPY) -O binary build/$(PROJECT_NAME).elf build/$(PROJECT_NAME).bin

build/$(PROJECT_NAME)_bin.h: build/$(PROJECT_NAME).bin | build
	xxd -i build/$(PROJECT_NAME).bin > $@
	sed -i 's/build_//g' $@
