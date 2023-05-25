obj-m += hello_mailbox.o

PWD := $(CURDIR)

# semicolons required to prevent launching a new shell
all:
	cd riscv; make -f Makefile; cd ..;
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	cd riscv; make -f Makefile clean; cd ..;
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
