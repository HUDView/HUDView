#!/bin/bash

# Requires the line /usr/share/X11/xorg.conf.d/99-fbturbo.conf pertaining to
# /dev/fb1 to be commented out

#sudo modprobe fbtft_device # FBTFT kernel module
sudo modprobe spi-bcm2835 # SPI kernel module
sudo modprobe fbtft_device name=adafruit18 # 1.8in screen kernel module
con2fbmap 1 1 # Maps console 1 and /dev/fb1
# https://github.com/notro/fbtft/wiki/Framebuffer-use
fbcp & #Copy primary FB to secondary FB
python3 ~/HUDView/Camera/preview.py # Run camera
