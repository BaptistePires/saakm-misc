KERNEL_SRC=~/these/kernels/mon-repo-ipa
obj-m += fifo_one_queue.o

all :
	make -C $(KERNEL_SRC) M=$$PWD modules
	cp fifo_one_queue.ko ~/these/kernels/mon-repo-ipa/kernel/sched/ipanema/

user:
	gcc main.c -o main

clean:
	make -C $(KERNEL_SRC) M=$$PWD clean
