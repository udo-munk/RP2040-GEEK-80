#include "pico/platform.h"
#include "GUI_Paint.h"
#include "Debug.h"

void __not_in_flash_func(Paint_BmpWindows)(uint8_t x, uint8_t y,
					   const uint8_t *pBmp,
					   uint8_t chWidth, uint8_t chHeight)
{
	uint16_t i, j, byteWidth = (chWidth + 7) / 8;

	for (j = 0; j < chHeight; j++)
		for (i = 0; i < chWidth; i++)
			if (*(pBmp + j * byteWidth + i / 8) &
			    (0x80 >> (i & 7)))
				Paint_SetPixel(x + i, y + j, 0xffff);
}
