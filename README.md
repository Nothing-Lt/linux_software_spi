# Kernel module for software SPI

Working with beaglebone balck, kernel version 4.19.94-ti-r55
Target is let it working with a SD card.

Since it is a software emulated SPI, so the pin configuration will be considered.
pin configuration is done with the kernel module parameter, cs pin is done with ioctl function.
Example of kernel module parameter:

sudo insmod sw_spi.ko mosi_pin=69 miso_pin=68 sck_pin=66 cpol=0 cpha=0