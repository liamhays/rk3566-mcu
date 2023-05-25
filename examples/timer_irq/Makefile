obj-m += timer_irq.o

PWD := $(CURDIR)

all:
	make riscv/Makefile
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make riscv/Makefile clean
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
