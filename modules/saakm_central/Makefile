KERNEL_SRC=~/these/kernels/saakm-kernel
obj-m += saakm_central.o


all :
	CC=gcc-13 make -C $(KERNEL_SRC) M=$$PWD modules

clean:
	make -C $(KERNEL_SRC) M=$$PWD clean

compile_commands:
	bear -- make -C $(KERNEL_SRC) M=$PWD CC=gcc-13 modules