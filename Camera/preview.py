from picamera import PiCamera
from time import sleep

cam = PiCamera()
cam.rotation = 90
cam.start_preview()

while True:
    sleep(2)
