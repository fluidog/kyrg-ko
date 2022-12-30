## Makefile for the Linux kernel module

MODULE := kyrg
FILES := main.c kallsyms-lookup-name.c periodic-timer.c hash.c access-process-memory.c \
		kyrg-core.c rg-processes.c rg-modules.c rg-kernel.c fs.c

ifdef KERNELRELEASE

obj-m += $(MODULE).o
$(MODULE)-m := $(FILES:%.c=%.o)

else

ifndef KDIR
KDIR := /lib/modules/$(shell uname -r)/build
endif

build: $(FILE)
	make -C $(KDIR) M=$(shell pwd) modules
	make -C test/ build
clean:
	make -C $(KDIR) M=$(shell pwd) clean
	make -C test/ clean
	rm -f $^

install:
	mkdir -p $(DESTDIR)/opt/kyrg
	install -m 644 $(MODULE).ko  $(DESTDIR)/opt/kyrg

test:
	insmod $(MODULE).ko
	make -C test/ test
	rmmod $(MODULE)

include rpm.Makefile

.PHONY: build clean test install
endif
