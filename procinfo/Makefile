MODULENAME=procinfo

obj-m += $(MODULENAME).o

modules:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean

install:
	sudo insmod $(MODULENAME).ko
	sudo chmod +r /dev/procinfo

uninstall:
	sudo rmmod $(MODULENAME)