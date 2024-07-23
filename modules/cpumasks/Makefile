KERNEL_SRC=~/these/kernels/mon-repo-ipa
obj-m += cpumasks.o

all :
	make -C $(KERNEL_SRC) M=$$PWD modules

clean:
	make -C $(KERNEL_SRC) M=$$PWD clean
