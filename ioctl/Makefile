EXTRA_CFLAGS += -Wall -march=armv5
export ${EXTRA_CFLAGS}

obj-m := totoro.o
k16_mod-objs:= totoro.o

MODULE_NAME = totoro
KVER = 2.6.33

all:
	make -C /sam/linux-$(KVER) M=${PWD} modules
clean:
	make -C /sam/linux-$(KVER) M=${PWD} clean
install:
	install -p -m 644 $(MODULE_NAME).ko ${SYSROOT}/lib/modules/$(KVER)/kernel/drivers/
	/sbin/depmod -a $(KVER)
