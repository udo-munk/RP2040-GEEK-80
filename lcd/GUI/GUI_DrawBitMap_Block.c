#include "GUI_Paint.h"
#include "Debug.h"

void Paint_DrawBitMap_Block(const uint8_t *image_buffer, uint8_t Region)
{
	uint16_t x, y;
	uint32_t Addr = 0;

	for (y = 0; y < Paint.HeightByte; y++) {
		for (x = 0; x < Paint.WidthByte; x++) {
			/* 8 pixel = 1 byte */
			Addr = x + y * Paint.WidthByte;
			Paint.Image[Addr] =
				(uint8_t) image_buffer[Addr +
						       (Paint.HeightByte) *
						       Paint.WidthByte *
						       (Region - 1)];
		}
	}
}
