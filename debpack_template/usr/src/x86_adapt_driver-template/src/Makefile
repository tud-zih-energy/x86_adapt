
obj-m += x86_adapt_driver.o

KBUILD_EXTRA_SYMBOLS := ../definition_driver/x86_adapt_defs.o
 
all:
	$(eval DIRX = $(strip $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))))
	make -C $(DIRX)/../definition_driver all
	make -C /lib/modules/$(shell uname -r)/build M=$(DIRX) modules
 
clean:
	$(eval DIRX = $(strip $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))))
	make -C /lib/modules/$(shell uname -r)/build M=$(DIRX) clean
