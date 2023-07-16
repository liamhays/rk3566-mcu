obj-m += read_registers.o

PWD := $(CURDIR)

# semicolons required to prevent starting a new shell
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
