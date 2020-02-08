#include <stdbool.h>							// C standard unit for bool, true, false
#include <stdint.h>								// C standard unit for uint32_t etc
#include "spi.h"								// SPI device unit as we will be using SPI
#include "gpio.h"								// We need access to GPIO to resetup reset pin
#include "Font8x16.h"							// Font 16x8 bitmap data
#include "ssd1327.h"							// This units header

static const uint8_t ssd1327_init[35] = 
{
	0xae,				// turn off oled panel off while we change  settings
	0x15, 0x00, 0x7f,	// set column address start=0 end=127
	0x75, 0x00, 0x7f,	// set row address start=0 end=127
	0x81, 0x80,         // set contrast control to 50%
	0xa0, 0x51,         // gment remap  ... set orientation to normal
	0xa1, 0x00,         // start line 0
	0xa2, 0x00,         // display offset 0
	0xa4,               // rmal display
	0xa8, 0x7f,         // set multiplex ratio
	0xb1, 0xf1,         // set phase length
	0xb3, 0x00,         // set dclk: 80Hz:0xc1 90Hz:0xe1   100Hz:0x00   110Hz:0x30 120Hz:0x50   130Hz:0x70
	0xab, 0x01,         // Enable VDD 
	0xb6, 0x0f,         // Set Second Pre-charge period
	0xbe, 0x0f,         // set VCOMH Voltage
	0xbc, 0x08,         // Set pre-charge voltage level.
	0xd5, 0x62,         // Function selection B
	0xfd, 0x12,         // Set Command Lock (unlocked)
	0xaf,               // Turn OLED screen back on
};

typedef struct ssd1327_device
{
	SPI_HANDLE spi;				// SPI Handle for device
	uint16_t screenwth;			// Screen width
	uint16_t screenht;			// Screen ht
	uint16_t fontwth;			// Current font width
	uint16_t fontht;			// Current font height
	uint16_t fontstride;		// Bytes between characters in font
	uint8_t* fontdata;			// Pointer to current font data
} SSD1327;

/* Global table of ssd1327 devices.  */
static SSD1327 tab[1] = { {0} };

/*-[ SSD1327_Open ]---------------------------------------------------------}
. Open access to an SSD1327 on the given SPI device handle. The SPI device
. should be opened with desired speed settings and SPI_MODE3 before call.
. It is also assumed a valid reset cycle on reset pin was completed.
. RETURN: true SSD1327 for success, false for any failure
.--------------------------------------------------------------------------*/
bool SSD1327_Open (SPI_HANDLE spi)
{
	if (tab[0].spi == 0)											// Make sure device is not already open 
	{
		tab[0].spi = spi;											// Hold the SPI Handle
		tab[0].screenwth = 128;										// Set screen width
		tab[0].screenht = 128;										// Set screen height
		tab[0].fontwth = 8;											// Font width = 8
		tab[0].fontht = 16;											// Font width = 16
		tab[0].fontstride = 16;										// Font stride = 16 bytes per character
		tab[0].fontdata = (uint8_t*)&font_8x16_data[0];				// Pointer to font data
		SpiWriteAndRead(spi, (uint8_t*)&ssd1327_init[0], 0, 35, false);// Send initialize commands
		return true;												// Return success
	}
	return false;
}


/*-[ SSD1327_WriteChar ]----------------------------------------------------}
. Writes the character in the current font to the screen at position (x,y)
. **** Note x can only be even value 0,2,4 etc due to two pixel per byte 
. format and me being to lazy to buffer screen. If you do ask for an odd X
. value it will write at the value one less so x = 3 would write at x = 2.
. Fonts also have restriction they must be of even width .. the demo font 
. being 8 pixels wide.
.--------------------------------------------------------------------------*/
void SSD1327_WriteChar (uint16_t x, uint16_t y, char Ch)
{
	if (tab[0].spi && tab[0].fontdata)								// Make sure device is open and we have fontdata
	{
		uint8_t* video_wr_ptr = (uint8_t*)((y * tab[0].screenwth/2) + (x/2));
		uint8_t* bp = &tab[0].fontdata[(uint8_t)Ch * tab[0].fontstride];// Load font bitmap pointer
		for (uint16_t y = 0; y < tab[0].fontht; y++)				// For each line in font height
		{
			uint8_t b = *bp;										// Fetch the first font byte	
			bp++;													// Move to next font byte	
			for (uint16_t x = 0; x < tab[0].fontwth/2; x++)			// We write two bytes at a time
			{
				uint8_t col = 0;
				if ((b & 0x80) == 0x80) col |= 0xF0;				// Set High pixel
				if ((b & 0x40) == 0x40) col |= 0x0F;				// Set low pixel
				video_wr_ptr[x] = col;								// Write pixel
				b = b << 2;											// Shift 2 bit left
				if ((((x + 1) % 4) == 0) && (x + 1) < tab[0].fontwth)
				{
					b = *bp;										// Fetch next font byte
					bp++;											// Move to next font byte
				}
			}
			video_wr_ptr += (tab[0].screenwth / 2);					// Next line down
		}
	}
}

/*-[ SSD1327_WriteText ]----------------------------------------------------}
. Writes the text in the current font to the screen at position (x,y)
. **** Note x can only be even value 0,2,4 etc due to two pixel per byte
. format and me being to lazy to buffer screen. If you do ask for an odd X
. value it will write at the value one less so x = 3 would write at x = 2.
. Fonts also have restriction they must be of even width .. the demo font
. being 8 pixels wide.
.--------------------------------------------------------------------------*/
void SSD1327_WriteText (uint16_t x, uint16_t y, char* txt)
{
	if (tab[0].spi && tab[0].fontdata && txt)						// Make sure device is open and we have fontdata and txt pointer
	{
		x &= 0xFFFE;												// Make sure x value even 
		while (*txt != 0)											// Not a c string terminate character
		{
			char ch = *txt++;										// Next character
			SSD1327_WriteChar(x, y, ch);							// Write the charter to screen
			x += tab[0].fontwth;									// Move to next character position
		}
	}
}