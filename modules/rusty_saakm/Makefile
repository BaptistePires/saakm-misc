KERNEL_SRC=~/these/kernels/ipanema
obj-m += rusty_saakm.o

all :
	make -C $(KERNEL_SRC) M=$$PWD modules
	cp rusty_saakm.ko ~/these/shared
	cp rusty_saakm.ko $(KERNEL_SRC)/kernel/sched/ipanema/

clean:
	make -C $(KERNEL_SRC) M=$$PWD clean
