#!/bin/bash


if [ -f "ArduCopter.bin" ]; then
	echo find 	ArduCopter.bin
	echo Remove 	ArduCopter.bin
	rm ArduCopter.bin
fi


cd libmaple/
make jtag
cd ../
cd ardupilot/ArduCopter/


make flymaple -j4
cd ../..
# cp /tmp/ArduCopter.build/ArduCopter.hex ./



if [ -f "/tmp/ArduCopter.build/ArduCopter.elf" ]; 
then
	echo make success
	echo arm-none-eabi-objcopy -O binary ArduCopter.elf ArduCopter.bin
	arm-none-eabi-objcopy -O binary /tmp/ArduCopter.build/ArduCopter.elf ArduCopter.bin
	echo objcopy success
	ls -hl ArduCopter.bin
else
	echo make error
	echo objcopy fail
fi

