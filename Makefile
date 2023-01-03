## Makefile for the Linux kernel module

TARGET := kyrg
FILES := main.c kallsyms-lookup-name.c periodic-timer.c hash.c access-process-memory.c \
		kyrg-core.c rg-processes.c rg-modules.c rg-kernel.c fs.c

ifdef KERNELRELEASE

obj-m += $(TARGET).o
$(TARGET)-m := $(FILES:%.c=%.o)

else

KDIR ?= /lib/modules/$(shell uname -r)/build

build: $(FILE)
	make -C $(KDIR) M=$(shell pwd) modules

clean:
	make -C $(KDIR) M=$(shell pwd) clean
	rm -f $^

install:
	mkdir -p $(DESTDIR)/opt/kyrg
	install -m 644 $(TARGET).ko  $(DESTDIR)/opt/kyrg


test:
	insmod $(TARGET).ko
	make -C test/ test
	rmmod $(TARGET)

build:build-test
clean:clean-test
build-test:
	make -C test/ build
clean-test:
	make -C test/ clean


include rpm.Makefile
clean:clean-pkg

.PHONY: build clean test install
endif
