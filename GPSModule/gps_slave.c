/*---------------------------------------------*\
 * Test interface for the Adafruit GPS Modules *
 * GPS Slave Modules                           *
 * @author: Marco Serrato                      *
\* --------------------------------------------*/

#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>
#include<termios.h>
#include<string.h>
#include<time.h>
#include<regex.h>

typedef struct gps_slave {
	char read_byte[1];
	int sentence_length;
	unsigned char sentence[512];
} gps_slave;

gps_slave GPS_SLAVE_DATA;

// State functions -------------------
void read_first_byte(unsigned char nbyte, gps_slave* s);
void read_nmea_sentence_header(unsigned char nbyte, gps_slave* s);
void read_sentence(unsigned char nbyte, gps_slave* s);
void read_checksum(unsigned char nbyte, gps_slave* s);
void validate_checksum(unsigned char nbyte, gps_slave* s);
// -----------------------------------

int serial_port;
char read_byte[2];
unsigned char buf;


void (*state_ptr)(unsigned char, gps_slave*) = read_first_byte;

// Read in first byte, if it is not the `$` char then ignore
void read_first_byte(unsigned char rbyte, gps_slave* slave) {
	if(rbyte == 0x24) {
		state_ptr=read_nmea_sentence_header;
	}	
}

// Read in the sentence header, in our case, should be GPRMC
void read_nmea_sentence_header(unsigned char rbyte, gps_slave* slave) {
	static unsigned char currBuf[6];
	static int myIndex = 0;
	if(myIndex < 5) {
		currBuf[myIndex] = rbyte;
		myIndex++;
	}
	else {
		if(strcmp(currBuf, "GPRMC") == 0) {
			memcpy(slave->sentence, currBuf, 5);
			slave->sentence[5] = 0x2c;
			state_ptr=read_sentence;
			myIndex=0;	
	  }
		else {
			state_ptr=read_first_byte;
			myIndex=0;
		}	
	}
}

// Read entire sentence - Read until *
void read_sentence(unsigned char rbyte, gps_slave* slave) {
	static unsigned char sentenceBuf[512];
	static int rsIndex = 0;

	if(rbyte == 0x2a) {
		state_ptr=validate_checksum;
		slave->sentence_length = rsIndex + 6;
		memcpy(slave->sentence + 6, sentenceBuf, rsIndex);
		rsIndex = 0;
		printf("%s\n", slave->sentence);
	}
	else {
		sentenceBuf[rsIndex] = rbyte;
		rsIndex++;
	}
}

void validate_checksum(unsigned char rbyte, gps_slave* slave) {
	static int checkSumInd = 0;
	static unsigned char vcCheck[3];
	if (checkSumInd < 1) {
		vcCheck[0] = rbyte;
		checkSumInd++;
	}
	else {	
		vcCheck[1] = rbyte;
		vcCheck[2] = '\0';
		int given;
		sscanf(vcCheck, "%x", &given);
		
		int checksum = 0;
		for(int i = 0; i < slave->sentence_length; i++) {
			checksum = checksum ^ ((int) slave->sentence[i]);	
		}
			
		if(given == checksum) {
			printf("Checksum Pass\n");
			state_ptr=read_first_byte;
		}
		else {
			printf("Checksum failed\n");
			state_ptr=read_first_byte;
		}
		checkSumInd=0;
	}
}

// Initialize the serial port
int initialize_serial() {
	serial_port = open("/dev/ttyAMA0", O_RDWR | O_NOCTTY | O_NDELAY);

	if(serial_port == -1) {
		printf("Cannot open /dev/ttyAMA0");
		return -1;
	}

	// Not sure what this is needed for but needed somehow	
	struct termios options;
	tcgetattr(serial_port, &options);
	options.c_cflag = B9600 | CS8 | CLOCAL | CREAD;		//<Set baud rate
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
	tcflush(serial_port, TCIFLUSH);
	tcsetattr(serial_port, TCSANOW, &options);

  return 0;
}

// Read a single byte from serial port
char fetch_byte() {
	char rbyte;
	int r; 
	do {	
		r = read(serial_port, (void*) &rbyte, 1);

	} while(r <= 0);
	return rbyte;

}

void get_full_message() {
	char nbyte = fetch_byte();
	state_ptr(nbyte, &GPS_SLAVE_DATA);	
}

int main() {	
	initialize_serial();
	size_t s = 255;	
	char* buffer;


	while(1) {
		get_full_message();
		usleep(1000);

	}

	close(serial_port);

	return 0;
		
}
