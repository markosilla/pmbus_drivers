TARGET		:= $(shell uname -r)
KERNEL_MODULES	:= /lib/modules/$(TARGET)
KERNEL_BUILD	:= $(KERNEL_MODULES)/build
DRIVER 		:= pmbus pmbus_core zl6100

obj-m		:= $(patsubst %,%.o,$(DRIVER))
obj-ko		:= $(patsubst %,%.ko,$(DRIVER))

all: 
	make -C $(KERNEL_BUILD)	M=$(PWD) modules
	#make -C /lib/modules/`uname -r`/build	M=$(PWD) modules_install

clean:

	make -C $(KERNEL_BUILD)	M=$(PWD) clean
