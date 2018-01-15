#include "lpc17xx.h"
#include "stdio.h"

#define p13 (P0_15)
#define SBIT_WordLength (0x00)
#define SBIT_DLAB (0x07)
#define SBIT_FIFO (0x00)
#define SBIT_RxFIFO (0x01)
#define SBIT_TxFIFO (0x02)
#define WAIT_TIME (1)
#define I2C_READ_ADDRESS (0x3B)
#define I2C_WRITE_ADDRESS (0x3A)
#define UINT8_MAX (255)

//SPI spi(p5,NC,p7,p8);
//I2C i2c(p28, p27);

int main() {
	uint32_t penis = 24000000 / (16*9600);
	uint32_t penis_2 = 0x00;
	//LPC_SC->PCONP &= ~0x000000F0;
	//LPC_SC->PCONP |= 0x00000010; 
	//LPC_SC->PCLKSEL0 = 0xFFFFFCFF;
	LPC_UART1->FCR = (1<<SBIT_FIFO) | (1<<SBIT_RxFIFO) | (1<<SBIT_TxFIFO);    
	LPC_UART1->LCR = (0x03<<SBIT_WordLength) | (1<<SBIT_DLAB);
	LPC_UART1->DLL = penis;
	LPC_UART1->DLM = penis_2;	
	LPC_UART1->LCR &= ~0x000000F0;
	LPC_PINCON->PINSEL0 &= ~0xF0000000;
	LPC_PINCON->PINSEL0 |= 0x40000000;
	LPC_PINCON->PINMODE0 &= ~0xF0000000;
	LPC_PINCON->PINMODE0 |= 0xC0000000;
	
	while (1) {
		LPC_UART1->THR = 0xF;
	}
}
