build: gps_slave.c
	gcc -o gps_slave gps_slave.c
clean:
	rm -f gps_slave
upload: test.c Makefile
	scp test.c Makefile pi@172.20.10.14:/home/pi/
