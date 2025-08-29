KERNEL_SRC=~/these/kernels/ipanema-kernel
obj-m += fifo.o

all :
	make -C $(KERNEL_SRC) M=$$PWD modules
	cp fifo.ko ~/these/shared
	cp fifo.ko $(KERNEL_SRC)/kernel/sched/ipanema/fifo.ko

clean:
	make -C $(KERNEL_SRC) M=$$PWD clean
