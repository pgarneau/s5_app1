#include "mbed.h"
#include "stdio.h"

#define PI (3.1416)
#define WAIT_TIME (1)
#define I2C_READ_ADDRESS (0x3B)
#define I2C_WRITE_ADDRESS (0x3A)
#define UINT14_MAX (16383)

SPI spi(p5,NC,p7,p8);
I2C i2c(p28, p27);

int main() {
	char accel_data[2] = {0};
	float z_offset = 0;
	int16_t z_accel = 0;

	const char init_cmd[2] = {0x2A, 0x01};
	const char z_msb[1] = {0x05};
	
	// Configuration de la lib SPI
	spi.format(8,0);
  spi.frequency(100000);
	spi.write(0x76);
	
	// Configuration I2C
	i2c.frequency(100000);

	while (1) {
		memset(accel_data, 0, 1);

		// Vise registre z_msb et read en sequence de 2 pour MSB et LSB
		i2c.write(I2C_WRITE_ADDRESS, z_msb, 1, true);
		i2c.read(I2C_READ_ADDRESS, accel_data, 2);
		
		// Normalisation de la valeur de l'acceleration de Z
    z_accel = (accel_data[0] << 6) | (accel_data[1] >> 2);
    if (z_accel > UINT14_MAX/2)
        z_accel -= UINT14_MAX;
		
		float z_accel_result = (z_accel/4096.0);
		
		if (z_accel_result > 1) {
			z_accel_result = 1;
		}
		if (z_accel_result < -1) {
			z_accel_result = -1;
		}
		
		// Transformation valeur d'accel en z en angle
		float z_angle = std::acos(z_accel_result);
		
		// Assurer que l'angle est max 90 degree
		if (z_angle > PI / 2) {
			z_angle = PI - z_angle;
		}
		
		// Transformation rad en degree
		z_angle = z_angle * 180 / PI;
		
		// Remplissage tableau de char pour afficher valeur angle
		char string_result[3] = {0};		
		uint8_t string_size = 0;
	  string_size = sprintf(string_result, "%d",(int)z_angle - (int)z_offset);
				
		// Afficher angle
		spi.write(0x76);
		for (int index = 0; index < string_size; index++) {
			spi.write(0x79);
			spi.write(4 - string_size + index);
			spi.write(string_result[index]);
		}
		
		wait(0.5);
	}
}
