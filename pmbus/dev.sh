#!/bin/sh

echo "PULL"
git pull

echo "BUILD"
make
#sudo make -C /lib/modules/`uname -r`/build M=$PWD modules
#sudo make -C /lib/modules/`uname -r`/build M=$PWD modules_install

echo "REMOVE MODULES"
sudo rmmod zl6100.ko
sudo rmmod pmbus.ko
sudo rmmod pmbus_core.ko
sudo modprobe -r hwmon

echo "ADD MODULES"
sudo modprobe hwmon
sudo insmod pmbus_core.ko
sudo insmod pmbus.ko
sudo insmod zl6100.ko

echo "FINISHED"
