#include "GUI_Paint.h"
#include "Debug.h"

/******************************************************************************
function:	Display monochrome bitmap
parameter:
    image_buffer : A picture data converted to a bitmap
info:
    Use a computer to convert the image into a corresponding array,
    and then embed the array directly into Imagedata.cpp as a .c file.
******************************************************************************/
void Paint_DrawBitMap(const uint8_t *image_buffer)
{
	uint16_t x, y;
	uint32_t Addr = 0;

	for (y = 0; y < Paint.HeightByte; y++) {
		for (x = 0; x < Paint.WidthByte; x++) {
			/* 8 pixel = 1 byte */
			Addr = x + y * Paint.WidthByte;
			Paint.Image[Addr] = (uint8_t) image_buffer[Addr];
		}
	}
}
