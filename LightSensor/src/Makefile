all:
	gcc -Wall -c tsl2561.c -o tsl2561.o -lm
	gcc -Wall tsl2561.o main.c -o run_light_sensor -lm

clean:
	rm tsl2561.o run_light_sensor &> /dev/null
