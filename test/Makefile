
build: build-process build-module
clean: clean-process clean-module
test: test-process test-module test-kernel

app := test-kyrg-process

build-process: $(app)

test-process: build-process
	./$(app) &
	echo "add `pidof -s $(app)` 1" > /sys/kernel/security/kyrg/policy_process
	echo 1 > /sys/kernel/security/kyrg/immediate
	# It will modify the code segment by itself. When the specified signal is received.
	kill -10 `pidof -s $(app)`
	echo 1 > /sys/kernel/security/kyrg/immediate

clean-process:
	rm -f *.o $(app)

## Makefile for the Linux kernel module

MODULE := test-kyrg
FILES := test-kyrg-module.c

ifdef KERNELRELEASE

obj-m += $(MODULE).o
$(MODULE)-m := $(FILES:%.c=%.o)

else

ifndef KDIR
KDIR := /lib/modules/$(shell uname -r)/build
endif

build-module:
	make -C $(KDIR) M=$(shell pwd) modules
clean-module:
	make -C $(KDIR) M=$(shell pwd) clean

test-module: MODPARAMS := name=do_rg
test-module: MOD := kyrg
test-kernel: MODPARAMS := name=vprintk
test-kernel: MOD := kernel

test-module test-kernel:
	echo "add $(MOD)" > /sys/kernel/security/kyrg/policy_module
	echo 1 > /sys/kernel/security/kyrg/immediate
	# This ko will modify the code segment of the kyrg module.
	insmod $(MODULE).ko $(MODPARAMS)
	echo 1 > /sys/kernel/security/kyrg/immediate
	rmmod $(MODULE)
	echo "del $(MOD)" > /sys/kernel/security/kyrg/policy_module

endif