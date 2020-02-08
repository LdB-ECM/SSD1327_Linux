#define _DEFAULT_SOURCE
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <linux/spi/spidev.h> // Needed for SPI_MODE_3

#include "gpio.h"
#include "spi.h"
#include "ssd1327.h"


static GPIO_HANDLE gpio = 0;
static SPI_HANDLE spi = 0;

int main (void) 
{
	gpio = GPIO_Open(0x0, 0x1000);									// Open GPIO access
	if (gpio == 0)													// Check it opened
	{
		fprintf(stderr, "Error setting up GPIO\n");
		return 1;
	}

	GPIO_Setup(gpio, 25, GPIO_OUTPUT);								// GPIO25 to output mode .. reset pin for SSD1327
	GPIO_Output(gpio, 25, 1);										// Set to high

	spi = SpiOpenPort(0, 8, 10000000, SPI_MODE_3, false);			// Initialize SPI 0 for SSD1327 10Mhz, SPI_MODE3
	if (spi == NULL)												// Check SPI opened
	{
		fprintf(stderr, "SPI device could not open\n");
		return 1;
	}

	GPIO_Output(gpio, 25, 0);										// SSD1327 reset low
	usleep(10);														// sleep for 10uS .. reset is 2uS just being safe
	GPIO_Output(gpio, 25, 1);										// SSD1327 reset back high

	if (SSD1327_Open(spi))											// Open the SSD1327 which sends initialize string
	{
		SSD1327_WriteText(0, 0,  "Hello World");
		SSD1327_WriteText(0, 16, "  SSD1327  ");
		SSD1327_WriteText(0, 32, "Should Work");
	}
	else {
		fprintf(stderr, "SSD1327 device could not open\n");
		return 1;
	}

	getchar();														// Wait of a keypress

	return (0);														// Exit program wioth no error
}


