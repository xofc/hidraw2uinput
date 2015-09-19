# hidraw2uinput
Mouse emulation for the 52Pi 5 inch touchscreen (with an USB interface)

This small program reads /dev/hidraw0 data sent by the 52Pi 5 inch touchscreen
and then emulates mouse events on /dev/uinput.

All the parameters are hardcoded (one has to modify the program to adapt
* /dev/hidraw* (if not 0)
* the calibration.

It is inspired by
https://github.com/derekhe/wavesahre-7inch-touchscreen-driver
which is writen in Python3 with some dependencies
and
http://thiemonge.org/getting-started-with-uinput
which explains how to do it in 'C'.

It should probably be run as superuser from /etc/rc.local

----
dmesg gives
* idVendor=0eef, idProduct=0005
* hid-generic 0003:0EEF:0005.0004: hiddev0,hidraw0: USB HID v1.10 Device [RPI_TOUCH By ZH851] on usb-20980000.usb-1.2/input0
lsusb -t
says that the driver is 'usbhid'

Reading/dumping /dev/hidraw0, one gets 22 bytes by event; something like :

170   1   1 115  14  73 187   0   0   0   0   0   0...

* A start marker (170 decimal)
* touch status 1/0
* hi- low- X coordinate
* hi- low- Y coordinate
* An end marker (187 decimal)


