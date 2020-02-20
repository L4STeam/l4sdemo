TARGET := sch_taildrop
QDISC  := sch_taildrop

obj-m += $(TARGET).o
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

unload:
	-rmmod $(TARGET).ko

load:
	insmod $(TARGET).ko 
