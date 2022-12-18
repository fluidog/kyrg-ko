## Makefile for the Linux kernel module

MODULE := kyrg
FILES := main.c kallsyms-lookup-name.c periodic-timer.c hash.c access-process-memory.c

ifdef KERNELRELEASE

obj-m += $(MODULE).o
$(MODULE)-m := $(FILES:%.c=%.o)

else

ifndef KDIR
KDIR := /lib/modules/$(shell uname -r)/build
endif

build: $(FILE)
	make -C $(KDIR) M=$(shell pwd) modules
clean:
	make -C $(KDIR) M=$(shell pwd) clean
	rm -f $^

run:
	insmod $(MODULE).ko
	rmmod $(MODULE)
endif
