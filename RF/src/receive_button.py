import RPi.GPIO as GPIO

GPIO.setmode(GPIO.BCM)
GPIO.setup(27, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)
GPIO.add_event_detect(27, GPIO.RISING)

def button_press_callback():
    print "BUTTON PUSHED"

GPIO.add_event_callback(27, button_press_callback())
