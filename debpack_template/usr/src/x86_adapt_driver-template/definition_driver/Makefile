
obj-m += x86_adapt_defs.o

all:
	$(eval DIRX = $(strip $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))))
	make -C /lib/modules/$(shell uname -r)/build M=$(DIRX) modules
	
clean:
	$(eval DIRX = $(strip $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))))
	make -C /lib/modules/$(shell uname -r)/build M=$(DIRX) clean
