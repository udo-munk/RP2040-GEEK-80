#include "pico/platform.h"
#include "GUI_Paint.h"
#include "Debug.h"

/******************************************************************************
function: Select image depth
parameter:
    depth : 1, 2, 4, 8, 12, 16
******************************************************************************/
void __not_in_flash_func(Paint_SetDepth)(uint8_t depth)
{
	if (depth == 1) {
		Paint.Depth = depth;
		Paint.WidthByte = (Paint.WidthMemory % 8 == 0) ?
				  (Paint.WidthMemory / 8) :
				  (Paint.WidthMemory / 8 + 1);
	} else if (depth == 2) {
		Paint.Depth = depth;
		Paint.WidthByte = (Paint.WidthMemory % 4 == 0) ?
				  (Paint.WidthMemory / 4) :
				  (Paint.WidthMemory / 4 + 1);
	} else if (depth == 4) {
		Paint.Depth = depth;
		Paint.WidthByte = (Paint.WidthMemory % 2 == 0) ?
				  (Paint.WidthMemory / 2) :
				  (Paint.WidthMemory / 2 + 1);
	} else if (depth == 8) {
		Paint.Depth = depth;
		Paint.WidthByte = Paint.WidthMemory;
	} else if (depth == 12) {
		Paint.Depth = depth;
		Paint.WidthByte = ((Paint.WidthMemory + 1) / 2) * 3;
	} else if (depth == 16) {
		Paint.Depth = depth;
		Paint.WidthByte = Paint.WidthMemory * 2;
	} else {
		Debug("Set depth input parameter error\r\n");
		Debug("Depth only support: 1 2 4 8 12 16\r\n");
	}
}
