KERNELDIR := /home/ubuntu/SOC_UBOOT/linux-imx-rel_imx_4.1.15_2.1.0_ga

CURRENT_PATH := $(shell pwd)

obj-m := chardevbase.o

build : kernel_modules

kernel_modules :
	$(MAKE) -C $(KERNELDIR) M=$(CURRENY_PATH) modules

clean : 
	$(MAKE) -C $(KERNELDIR) M=$(CURRENY_PATH) clean
