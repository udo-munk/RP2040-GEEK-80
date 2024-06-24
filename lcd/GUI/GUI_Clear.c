#include <string.h>
#include "pico/platform.h"
#include "GUI_Paint.h"
#include "Debug.h"

/******************************************************************************
function: Clear the color of the picture
parameter:
    Color : Painted colors
******************************************************************************/
void __not_in_flash_func(Paint_Clear)(uint16_t Color)
{
	uint32_t Addr;
	uint16_t X, Y;
	int i;
	uint8_t Bytes[3];

	if (Paint.Depth == 1) {
		Color &= 0x1;
		Color |= Color << 1;
		Color |= Color << 2;
		Color |= Color << 4;
	} else if (Paint.Depth == 2) {
		Color &= 0x3;
		Color |= Color << 2;
		Color |= Color << 4;
	} else if (Paint.Depth == 4) {
		Color &= 0xf;
		Color |= Color << 4;
	}
	if (Paint.Depth <= 8 || Color == BLACK || Color == WHITE) {
		memset(Paint.Image, Color & 0xff,
		       (size_t) Paint.HeightByte * Paint.WidthByte);
	} else if (Paint.Depth == 12) {
		Bytes[0] = (Color >> 4) & 0xff;
		Bytes[1] = ((Color & 0xf) << 4) | ((Color >> 8) & 0xf);
		Bytes[2] = Color & 0xff;
		Addr = 0;
		i = 0;
		for (X = 0; X < Paint.WidthByte; X++) {
			Paint.Image[Addr++] = Bytes[i++];
			if (i == 3)
				i = 0;
		}
		for (Y = 1; Y < Paint.HeightByte; Y++) {
			memcpy(&Paint.Image[Addr], Paint.Image,
			       Paint.WidthByte);
			Addr += Paint.WidthByte;
		}
	} else if (Paint.Depth == 16) {
		Addr = 0;
		for (X = 0; X < Paint.WidthByte / 2; X++) {
			Paint.Image[Addr++] = (Color >> 8) & 0xff;
			Paint.Image[Addr++] = Color & 0xff;
		}
		for (Y = 1; Y < Paint.HeightByte; Y++) {
			memcpy(&Paint.Image[Addr], Paint.Image,
			       Paint.WidthByte);
			Addr += Paint.WidthByte;
		}
	}
}
