#include "mbed.h"
#include "stdio.h"

#define SBIT_WordLength (0x00)
#define SBIT_DLAB (0x07)
#define SBIT_FIFO (0x00)
#define SBIT_RxFIFO (0x01)
#define SBIT_TxFIFO (0x02)
#define WAIT_TIME (1)
#define I2C_READ_ADDRESS (0x3B)
#define I2C_WRITE_ADDRESS (0x3A)
#define DLM_VALUE (0x00)
#define UINT8_MAX (255)

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
	
I2C i2c(p28, p27);

int main() {
	const char init_cmd[2] = {0x2A, 0x01};
	const char x_msb[1] = {0x01};
	
	// Configuration I2C
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
	*(volatile unsigned int*)0x4002C044 = *(volatile unsigned int*)0x4002C044 & ~0x00F00000;
	*(volatile unsigned int*)0x4002C044 = *(volatile unsigned int*)0x4002C044 | 0x00300000;
	
	// GPIO FIO0PIN3
	*(volatile unsigned int*)0x2009C003 = *(volatile unsigned int*)0x2009C003 | 0x00;

	char accel_data[6] = {0};
	uint8_t z_offset = 0;
	while (1) {
		memset(accel_data, 0, 1);

		// Vise registre x_msb et read en sequence
		i2c.write(I2C_WRITE_ADDRESS, x_msb, 1, true);
		i2c.read(I2C_READ_ADDRESS, accel_data, 6);
		
		for (int index = 0; index < 5; index += 2) {
			if (accel_data[index] > UINT8_MAX/2) {
				accel_data[index] -= UINT8_MAX;
			}
		}
		
		int8_t x_result = (accel_data[0] * 360) / 255;
		int8_t y_result = (accel_data[2] * 360) / 255;
		int8_t z_result = (accel_data[4] * 360) / 255;
		z_result = 90 - z_result;
		
		// Verification si bouton appuye pour remise a 0
		char binary_result[32] = {0};
		sprintf(binary_result,PRINTF_BINARY_PATTERN_INT32,PRINTF_BYTE_TO_BINARY_INT32(*(volatile unsigned int*)0x2009C017));
		if (binary_result[29] == '1') {
			z_offset = z_result;
		}
		
		char string_result[3] = {0};		
		uint8_t string_size = 0;
	    string_size = sprintf(string_result, "%d", z_result - z_offset);
		
		// Afficher angle
		*(volatile unsigned int*)0x40010000 = 0x76;
		for (int index = 0; index < string_size; index++) {
			*(volatile unsigned int*)0x40010000 = 0x79;
			*(volatile unsigned int*)0x40010000 = (4 - string_size + index);
			*(volatile unsigned int*)0x40010000 = (string_result[index]);
		}
		
		wait(0.5);
	}
}
