#include "mbed.h"
#include "stdio.h"

// Constantes necessaires pour code
#define PI (3.1416)
#define SBIT_WordLength (0x00)
#define SBIT_DLAB (0x07)
#define SBIT_FIFO (0x00)
#define SBIT_RxFIFO (0x01)
#define SBIT_TxFIFO (0x02)
#define WAIT_TIME (1)
#define I2C_READ_ADDRESS (0x3B)
#define I2C_WRITE_ADDRESS (0x3A)
#define DLM_VALUE (0x00)
#define UINT14_MAX (16383)

// Formateur de valeur hexa en binaire jusqu'a 32 bits
#define PRINTF_BINARY_PATTERN_INT8 "%c%c%c%c%c%c%c%c"
#define PRINTF_BYTE_TO_BINARY_INT8(i)    \
    (((i) & 0x80ll) ? '1' : '0'), \
    (((i) & 0x40ll) ? '1' : '0'), \
    (((i) & 0x20ll) ? '1' : '0'), \
    (((i) & 0x10ll) ? '1' : '0'), \
    (((i) & 0x08ll) ? '1' : '0'), \
    (((i) & 0x04ll) ? '1' : '0'), \
    (((i) & 0x02ll) ? '1' : '0'), \
    (((i) & 0x01ll) ? '1' : '0')

#define PRINTF_BINARY_PATTERN_INT16 \
    PRINTF_BINARY_PATTERN_INT8              PRINTF_BINARY_PATTERN_INT8
#define PRINTF_BYTE_TO_BINARY_INT16(i) \
    PRINTF_BYTE_TO_BINARY_INT8((i) >> 8),   PRINTF_BYTE_TO_BINARY_INT8(i)
#define PRINTF_BINARY_PATTERN_INT32 \
    PRINTF_BINARY_PATTERN_INT16             PRINTF_BINARY_PATTERN_INT16
#define PRINTF_BYTE_TO_BINARY_INT32(i) \
    PRINTF_BYTE_TO_BINARY_INT16((i) >> 16), PRINTF_BYTE_TO_BINARY_INT16(i)
	
// Configuration comm i2c
I2C i2c(p28, p27);

int main() {
	const char init_cmd[2] = {0x2A, 0x01};
	const char x_msb[1] = {0x01};
	const char z_msb[1] = {0x05};
	
	// Configuration I2C + initialisation
	i2c.frequency(100000);
	i2c.write(I2C_WRITE_ADDRESS, init_cmd, 2);
	
	// Configuration de communication UART
	int final_baud = 24000000 / (16*9600);
	
	// Configuration non obligatoire
	// LPC_SC->PCONP &= ~0x000000F0;
	// LPC_SC->PCONP |= 0x00000010; 
	// LPC_SC->PCLKSEL0 = 0xFFFFFCFF;
	
	// Configuration reception de data
	*(volatile unsigned int*)0x40010008 = (1<<SBIT_FIFO) | (1<<SBIT_RxFIFO) | (1<<SBIT_TxFIFO);  	
	*(volatile unsigned int*)0x4001000C = (0x03<<SBIT_WordLength) | (1<<SBIT_DLAB);
	
	// Configuration  baud rate
	*(volatile unsigned int*)0x40010000 = final_baud;
	*(volatile unsigned int*)0x40010004 = DLM_VALUE;
	
	// Remettre DLAB a 0 pour envoyer information
	*(volatile unsigned int*)0x4001000C = *(volatile unsigned int*)0x4001000C & ~0x000000F0;
	
	// UART PinSelect
	*(volatile unsigned int*)0x4002C000 = *(volatile unsigned int*)0x4002C000 & ~0xF0000000;
	*(volatile unsigned int*)0x4002C000 = *(volatile unsigned int*)0x4002C000 | 0x40000000;
	
	// GPIO PinSelect
	//*(volatile unsigned int*)0x4002C004 = *(volatile unsigned int*)0x4002C004 & ~0xF0000000;
	//*(volatile unsigned int*)0x4002C004 = *(volatile unsigned int*)0x4002C004 | 0x40000000;

	// UART PinMode
	*(volatile unsigned int*)0x4002C040 = *(volatile unsigned int*)0x4002C040 & ~0xF0000000;
	*(volatile unsigned int*)0x4002C040 = *(volatile unsigned int*)0x4002C040 | 0xC0000000;
	
	// GPIO PinMode
	*(volatile unsigned int*)0x4002C044 = *(volatile unsigned int*)0x4002C044 & ~0x00FF0000;
	*(volatile unsigned int*)0x4002C044 = *(volatile unsigned int*)0x4002C044 | 0x003F0000;
	
	// GPIO FIO0PIN3
	*(volatile unsigned int*)0x2009C003 = *(volatile unsigned int*)0x2009C003 | 0x00;

	// Variables necessaires durant la boucle
	char accel_data[2] = {0};
	float z_offset = 0;
	int16_t z_accel = 0;
	
	uint8_t low_counter = 0;
	char protocol_data[7] = {0};
	uint8_t data_counter = 0;
	bool reading_state = false;
	uint8_t parity_counter = 0;
	uint8_t decimal_control = 0x00;
	while (1) {
		memset(accel_data, 0, 1);

		// Vise registre x_msb et read en sequence
		i2c.write(I2C_WRITE_ADDRESS, z_msb, 1, true);
		i2c.read(I2C_READ_ADDRESS, accel_data, 2);
		
		// Calcul du vecteur d'acceleration en z + normalisation
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
		
		// Transformation de l'accel en angle
		float z_angle = std::acos(z_accel_result);
		
		// S'assure que ca reste entre 0 et 90
		if (z_angle > PI / 2) {
			z_angle = PI - z_angle;
		}
		
		// Transforme rad en degree
		z_angle = z_angle * 180 / PI;
		
		// Verification si bouton appuye pour remise a 0
		char binary_result[32] = {0};
		sprintf(binary_result,PRINTF_BINARY_PATTERN_INT32,PRINTF_BYTE_TO_BINARY_INT32(*(volatile unsigned int*)0x2009C017));
		if (binary_result[29] == '1') {
			z_offset = z_angle;
		}
		
		// Transformation hex a tableau de char pour affichage
		char string_result[3] = {0};		
		uint8_t string_size = 0;
	  string_size = sprintf(string_result, "%d",(int)z_angle - (int)z_offset);
		
		// Detection d'un premier high sur la clock apres une periode de stop
		if (binary_result[31] == '1' && low_counter > 6) {
			reading_state = true;
			
			protocol_data[data_counter] = binary_result[30];
			data_counter++;
		}
		
		// Detection de low sur clock
		else if (binary_result[31] == '0') {
			//memset(protocol_data, 0, 7);
			reading_state = false;
			low_counter++;
		}
		
		// Si notre data est "rempli"
		if (data_counter == 7) {
			low_counter = 0;
			data_counter = 0;
			parity_counter = 0;
			char command = protocol_data[5];
			char bit2 = protocol_data[4];
			char bit1 = protocol_data[3];
			char bit0 = protocol_data[2];
			char parity = protocol_data[1];
			
			// Verification que le premier bit et dernier bit est 1 pour que le contenu soit valide
			if (protocol_data[0] == '1' && protocol_data[6] == '1') {
				// Calcul de parite
				for (uint8_t index = 1; index < 6; index++) {
					if (protocol_data[index] == '1') {
						parity_counter++;
					}
				}
				// Assurance que parite est valide dans le data
				if (parity_counter % 2 == 0) {
					// Traduction des commandes du guide etudiant en code pour UART
					if (bit0 == '0' && bit1 == '0' && bit2 == '0') {
						if (command == '1') {
							decimal_control &= ~0x0F;
							decimal_control |= 0x01;
						} else {
							decimal_control &= ~0x0F;
							decimal_control |= 0x00;
						}
						*(volatile unsigned int*)0x40010000 = 0x77;
						*(volatile unsigned int*)0x40010000 = decimal_control;
					}
					else if (bit0 == '0' && bit1 == '0' && bit2 == '1') {
						if (command == '1') {
							decimal_control &= ~0x0F;
							decimal_control |= 0x02;
						} else {
							decimal_control &= ~0x0F;
							decimal_control |= 0x00;
						}
						*(volatile unsigned int*)0x40010000 = 0x77;
						*(volatile unsigned int*)0x40010000 = decimal_control;
					}
					else if (bit0 == '0' && bit1 == '1' && bit2 == '0') {
						if (command == '1') {
							decimal_control &= ~0x0F;
							decimal_control |= 0x04;
						} else {
							decimal_control &= ~0x0F;
							decimal_control |= 0x00;
						}
						*(volatile unsigned int*)0x40010000 = 0x77;
						*(volatile unsigned int*)0x40010000 = decimal_control;
					}
					else if (bit0 == '0' && bit1 == '1' && bit2 == '1') {
						if (command == '1') {
							decimal_control &= ~0x0F;
							decimal_control |= 0x08;
						} else {
							decimal_control &= ~0xF0;
							decimal_control |= 0x00;
						}
						*(volatile unsigned int*)0x40010000 = 0x77;
						*(volatile unsigned int*)0x40010000 = decimal_control;
					}
					else if (bit0 == '1' && bit1 == '0' && bit2 == '0') {
						if (command == '1') {
							decimal_control &= ~0xF0;
							decimal_control |= 0x10;
						} else {
							decimal_control &= ~0xF0;
							decimal_control |= 0x00;
						}
						*(volatile unsigned int*)0x40010000 = 0x77;
						*(volatile unsigned int*)0x40010000 = decimal_control;
					}
					else if (bit0 == '1' && bit1 == '0' && bit2 == '1') {
						if (command == '1') {
							decimal_control &= ~0xF0;
							decimal_control |= 0x20;
						} else {
							decimal_control &= ~0xF0;
							decimal_control |= 0x00;
						}
						*(volatile unsigned int*)0x40010000 = 0x77;
						*(volatile unsigned int*)0x40010000 = decimal_control;

					}
					else if (bit0 == '1' && bit1 == '1' && bit2 == '0') {
						if (command == '1') {
							*(volatile unsigned int*)0x40010000 = 0x7A;
							*(volatile unsigned int*)0x40010000 = 0x64;
						} else {
							*(volatile unsigned int*)0x40010000 = 0x7A;
							*(volatile unsigned int*)0x40010000 = 0xF;
						}
					}
				}
			}
		}
		
		// Afficher angle
		memset(binary_result, 0, 32);
		sprintf(binary_result,PRINTF_BINARY_PATTERN_INT32,PRINTF_BYTE_TO_BINARY_INT32(*(volatile unsigned int*)0x40010014));
		if (binary_result[26] == '1') {
			//*(volatile unsigned int*)0x40010000 = 0x76;
			// Remettre afficheur vide sur chiffres
			*(volatile unsigned int*)0x40010000 = 0x7B;
			*(volatile unsigned int*)0x40010000 = 0x00;
			*(volatile unsigned int*)0x40010000 = 0x7C;
			*(volatile unsigned int*)0x40010000 = 0x00;
			*(volatile unsigned int*)0x40010000 = 0x7D;
			*(volatile unsigned int*)0x40010000 = 0x00;
			*(volatile unsigned int*)0x40010000 = 0x7E;
			*(volatile unsigned int*)0x40010000 = 0x00;
			// Afficher en ordre les valeurs
			for (int index = 0; index < string_size; index++) {
				*(volatile unsigned int*)0x40010000 = 0x79;
				*(volatile unsigned int*)0x40010000 = (4 - string_size + index);
				*(volatile unsigned int*)0x40010000 = (string_result[index]);
			}
		}
		
		// Synchronisation pour lire le data
		if (low_counter > 0) {
			wait(0.002);
		}
		else if (reading_state) {
			wait(0.004);
		}
		else {
			wait(0.002);
		}
	}
}
