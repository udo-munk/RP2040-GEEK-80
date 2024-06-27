#include "pico/platform.h"
#include "GUI_Paint.h"
#include "Debug.h"

/******************************************************************************
function: Draw Pixels
parameter:
    Xpoint : At point X
    Ypoint : At point Y
    Color  : Painted colors
******************************************************************************/
void __not_in_flash_func(Paint_SetPixel)(uint16_t Xpoint, uint16_t Ypoint,
					 uint16_t Color)
{
	uint16_t X, Y;
	uint32_t Addr;
	uint8_t Rdata;

	if (Xpoint >= Paint.Width || Ypoint >= Paint.Height) {
		Debug("Exceeding display boundaries\r\n");
		return;
	}

	switch (Paint.Rotate) {
	case 0:
		X = Xpoint;
		Y = Ypoint;
		break;

	case 90:
		X = Paint.WidthMemory - Ypoint - 1;
		Y = Xpoint;
		break;

	case 180:
		X = Paint.WidthMemory - Xpoint - 1;
		Y = Paint.HeightMemory - Ypoint - 1;
		break;

	case 270:
		X = Ypoint;
		Y = Paint.HeightMemory - Xpoint - 1;
		break;

	default:
		return;
	}

	switch (Paint.Mirror) {
	case MIRROR_NONE:
		break;

	case MIRROR_HORIZONTAL:
		X = Paint.WidthMemory - X - 1;
		break;

	case MIRROR_VERTICAL:
		Y = Paint.HeightMemory - Y - 1;
		break;

	case MIRROR_ORIGIN:
		X = Paint.WidthMemory - X - 1;
		Y = Paint.HeightMemory - Y - 1;
		break;

	default:
		return;
	}

	if (X >= Paint.WidthMemory || Y >= Paint.HeightMemory) {
		Debug("Exceeding display boundaries\r\n");
		return;
	}

	if (Paint.Depth == 1) {
		Addr = X / 8 + Y * Paint.WidthByte;
		Rdata = Paint.Image[Addr];
		if (Color % 2 == 0)
			Paint.Image[Addr] = Rdata & ~(0x80 >> (X % 8));
		else
			Paint.Image[Addr] = Rdata | (0x80 >> (X % 8));
	} else if (Paint.Depth == 2) {
		Addr = X / 4 + Y * Paint.WidthByte;
		Color = Color % 4;
		Rdata = Paint.Image[Addr] & (~(0xc0 >> ((X % 4) * 2)));
		Paint.Image[Addr] = Rdata | ((Color << 6) >> ((X % 4) * 2));
	} else if (Paint.Depth == 4) {
		Addr = X / 2 + Y * Paint.WidthByte;
		Color = Color % 16;
		Rdata = Paint.Image[Addr] & (~(0xf0 >> ((X % 2) * 4)));
		Paint.Image[Addr] = Rdata | ((Color << 4) >> ((X % 2) * 4));
	} else if (Paint.Depth == 8) {
		Addr = X + Y * Paint.WidthByte;
		Paint.Image[Addr] = Color & 0xff;
	} else if (Paint.Depth == 12) {
		Addr = (X / 2) * 3 + Y * Paint.WidthByte;
		if ((X % 2) == 0) {
			Paint.Image[Addr] = (Color >> 4) & 0xff;
			Rdata = Paint.Image[Addr + 1] & 0x0f;
			Paint.Image[Addr + 1] = ((Color & 0x0f) << 4) | Rdata;
		} else {
			Rdata = Paint.Image[Addr + 1] & 0xf0;
			Paint.Image[Addr + 1] = Rdata | ((Color >> 8) & 0x0f);
			Paint.Image[Addr + 2] = Color & 0xff;
		}
	} else if (Paint.Depth == 16) {
		Addr = X * 2 + Y * Paint.WidthByte;
		Paint.Image[Addr] = (Color >> 8) & 0xff;
		Paint.Image[Addr + 1] = Color & 0xff;
	}
}
