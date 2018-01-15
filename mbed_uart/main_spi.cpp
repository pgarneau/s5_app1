#include "mbed.h"

#define WAIT_TIME (1)

SPI spi(p5,NC,p7,p8);

int main() {
	
		spi.format(8,0);
    spi.frequency(100000);
	  spi.write(0x76);
		spi.write(0xf);
	  spi.write(0x7C);
		spi.write(7);
		spi.write(0x7D);
		spi.write(0xf);
}
