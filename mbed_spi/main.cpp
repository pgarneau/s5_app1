#include "mbed.h"
#include "stdio.h"

#define WAIT_TIME (1)
#define I2C_READ_ADDRESS (0x3B)
#define I2C_WRITE_ADDRESS (0x3A)
#define UINT8_MAX (255)

SPI spi(p5,NC,p7,p8);
I2C i2c(p28, p27);

int main() {
	const char INIT_CMD[2] = {0x2A, 0x01};
	const char x_msb[1] = {0x01};
	const char x_lsb[1] = {0x02};
	const char y_msb[1] = {0x03};
	const char y_lsb[1] = {0x04};
	const char z_msb[1] = {0x05};
	const char z_lsb[1] = {0x06};
	
	spi.format(8,0);
    spi.frequency(100000);
	spi.write(0x76);
	
	i2c.frequency(100000);
	char result[6] = {0}; //memset(result, '\0', sizeof(char)*1);
	while (1) {
		memset(result, 0, 1);
		int write_1 = i2c.write(I2C_WRITE_ADDRESS, INIT_CMD, 2);
		int write_2 = i2c.write(I2C_WRITE_ADDRESS, x_msb, 1, true);
		uint8_t success = i2c.read(I2C_READ_ADDRESS, result, 6);
		//for (uint8_t index = 0; index < 6; index++) {
		//	uint8_t success = i2c.read(I2C_READ_ADDRESS, &result[index], 1);
		//}
		
		if (result[0] > UINT8_MAX/2) {
			result[0] -= UINT8_MAX;
		}
		
		int8_t x_result = (result[0] * 365) / 255;
		int8_t y_result = result[2] / 10;
		int8_t z_result = result[4] / 10;
		
		char string_result[3] = {0};
				
		uint8_t banane = 0;
	    banane = sprintf(string_result, "%d", x_result);
		spi.write(0x76);
		for (int index = 0; index < banane; index++) {
			spi.write(0x79);
			spi.write(4 - banane + index);
			spi.write(string_result[index]);
		}
		wait(0.5);
	}
}
