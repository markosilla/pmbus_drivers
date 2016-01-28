#!/bin/sh

echo "PULL"
#git pull

echo "BUILD"
make clean
make
#sudo make -C /lib/modules/`uname -r`/build M=$PWD modules
#sudo make -C /lib/modules/`uname -r`/build M=$PWD pmbus_core.ko
#sudo make -C /lib/modules/`uname -r`/build M=$PWD pmbus.ko
#sudo make -C /lib/modules/`uname -r`/build M=$PWD zl6100.ko
#sudo make -C /lib/modules/`uname -r`/build M=$PWD modules_install

echo "COPY MODULES"
cp zl6100.ko /lib/modules/`uname -r`/
cp pmbus.ko /lib/modules/`uname -r`/
cp pmbus_core.ko /lib/modules/`uname -r`/

#echo "REMOVE MODULES"
sudo rmmod zl6100.ko
sudo rmmod pmbus.ko
sudo rmmod pmbus_core.ko
sudo modprobe -r hwmon

echo "ADD MODULES"
sudo modprobe hwmon
sudo modprobe pmbus_core
sudo modprobe pmbus
sudo modprobe zl6100 delay=30000
#sudo insmod pmbus_core.ko
#sudo insmod pmbus.ko
#sudo insmod zl6100.ko

echo "FINISHED"
