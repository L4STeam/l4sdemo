TARGET := sch_fq_codel_tst
QDISC  := sch_fq_codel_tst

obj-m += $(TARGET).o
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

unload:
	-rmmod $(TARGET).ko

load:
	insmod $(TARGET).ko 
