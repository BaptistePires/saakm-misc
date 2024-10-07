KERNEL_SRC=~/these/kernels/mon-repo-ipa
obj-m += fifo.o

all :
	make -C $(KERNEL_SRC) M=$$PWD modules
	cp fifo.ko ~/these/kernels/mon-repo-ipa/kernel/sched/ipanema/

clean:
	make -C $(KERNEL_SRC) M=$$PWD clean
