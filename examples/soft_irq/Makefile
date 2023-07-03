obj-m += soft_irq.o

PWD := $(CURDIR)

# semicolons required to prevent starting a new shell
all:
	cd riscv; make; cd ..;
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	cd riscv; make -f Makefile clean; cd ..;
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
