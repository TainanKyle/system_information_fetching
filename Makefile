obj-m += kfetch_mod_312551146.o

PWD := $(CURDIR)

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

load:
	sudo insmod kfetch_mod_312551146.ko

unload:
	sudo rmmod kfetch_mod_312551146.ko
