ifneq ($(KERNELRELEASE),)
# kbuild part of makefile
obj-m  := hrt_rw.o
else
# normal makefile
KDIR ?= /lib/modules/`uname -r`/build
#KBUILD_CFLAGS += -mno-mmx -mno-sse -mno-80387 -mno-fp-ret-in-387

default:
	$(MAKE) -C $(KDIR) M=$$PWD
clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean
endif