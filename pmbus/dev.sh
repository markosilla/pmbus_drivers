#!/bin/sh

echo "PULL"
#git pull

echo "BUILD"
make clean
make

echo "COPY MODULES"
cp bmr46x.ko /lib/modules/`uname -r`/
cp pmbus.ko /lib/modules/`uname -r`/
cp pmbus_core.ko /lib/modules/`uname -r`/

echo "REMOVE MODULES"
sudo modprobe -r bmr46x.ko
sudo modprobe -r pmbus.ko
sudo modprobe -r pmbus_core.ko
sudo modprobe -r hwmon

echo "ADD MODULES"
sudo modprobe hwmon
sudo modprobe pmbus_core
sudo modprobe pmbus
#default
sudo modprobe bmr46x delay=1000

echo "FINISHED"
