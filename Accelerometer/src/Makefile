all:
	gcc -Wall -c mma8451_pi.c -o mma8451_pi.o -lm
	gcc -Wall mma8451_pi.o main.c -o run_accelerometer -lm

clean:
	rm mma8451_pi.o run_accelerometer &> /dev/null
