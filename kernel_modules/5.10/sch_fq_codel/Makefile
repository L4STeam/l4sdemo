TARGET := sch_fq_codel_t
QDISC   := sch_fq_codel_t

obj-m += $(TARGET).o
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

unload:
	-rmmod $(TARGET).ko

load:
	insmod $(TARGET).ko 
