KERNEL_SRC=~/these/kernels/ipanema
obj-m += dicscover_topology.o

all :
	make -C $(KERNEL_SRC) M=$$PWD modules

clean:
	make -C $(KERNEL_SRC) M=$$PWD clean
