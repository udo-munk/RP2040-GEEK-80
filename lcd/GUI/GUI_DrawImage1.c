#include "pico/platform.h"
#include "GUI_Paint.h"
#include "Debug.h"

void Paint_DrawImage1(const uint8_t *image, uint16_t xStart, uint16_t yStart,
		      uint16_t W_Image, uint16_t H_Image)
{
	int i, j;

	for (j = 0; j < H_Image; j++) {
		for (i = 0; i < W_Image; i++) {
			/* Exceeded part does not display */
			if (xStart + i < Paint.HeightMemory &&
			    yStart + j < Paint.WidthMemory)
				Paint_SetPixel(xStart + i, yStart + j,
					       (*(image + j * W_Image * 2 +
						  i * 2 + 1)) << 8 |
					       (*(image + j * W_Image * 2 +
						  i * 2)));
			/*
			 * Using arrays is a property of sequential storage,
			 * accessing the original array by algorithm
			 * j * W_Image * 2		Y offset
			 * i * 2			X offset
			 */
		}
	}
}
