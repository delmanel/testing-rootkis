Some info about installing a module in linux kernel

First create a makefile as below

Obj-m := testRootkit.o
KERNEL_DIR = /lib/modules/$(shell uname -r)/build
PWD = $(shell PWD)
all:
 $(MAKE) -C $(KERNEL_DIR) SUBDIRS=$(PWD)
clean:
 rm -rf *.o *.ko *.symvers *.mod.* *.order 
 
 in order to compile you must have linux-headers installed. 
 execute: apt-get install linux-headers-$(uname -r)
 in order to install them.
 
 MORE INFO: MODULE_LICENSE("GPL") is required in order to prevent kernel.
 Also, functions module_init() and module_exit() are essential. 
 see http://www.tldp.org/LDP/lkmpg/2.6/html/c38.html
