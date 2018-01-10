#include "mbed.h"

#define WAIT_TIME (1)

DigitalOut myled(LED1);

int main() {
	unsigned int input = 0;
	gpio_t gpio;
	gpio_init_in_ex(&gpio, P1_15, PullUp);
	
    while(1) {
		input = gpio_read(&gpio);
		if (input == 1) {
			myled = 1;
		} else {
			myled = 0;
		}
        wait(WAIT_TIME);
    }
}
