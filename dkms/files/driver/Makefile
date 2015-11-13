
obj-m += x86_adapt_driver.o
 
all:
	$(eval DIRX = $(strip $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))))
	make -C $(DIRX)/../definition_driver all
	cp $(DIRX)/../definition_driver/Module.symvers $(DIRX)
	make -C /lib/modules/$(shell uname -r)/build M=$(DIRX) modules
 
clean:
	$(eval DIRX = $(strip $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))))
	make -C /lib/modules/$(shell uname -r)/build M=$(DIRX) clean
