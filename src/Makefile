obj-m += sw_spi.o
sw_spi-objs := soft_spi.o lib_soft_spi.o
KERNEL_MAKEFILE_PLACE = ~/ti-linux-kernel-dev/KERNEL

# KERNEL_MAKEFILE_PLACE is the place for makefile after compiling the kernel,this can be 

# The following 2 steps is for preventing the probnlem during compile the kernel and kernel module.
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-

all:
	make -C $(KERNEL_MAKEFILE_PLACE) M=$(PWD) modules

clean:
	make -C $(KERNEL_MAKEFILE_PLACE) M=$(PWD) clean
