#include "GUI_Paint.h"
#include "Debug.h"

/******************************************************************************
function: Select image depth
parameter:
    depth : 1, 2, 4, 8, 12, 16
******************************************************************************/
void Paint_SetDepth(uint8_t depth)
{
	if (depth == 1) {
		Paint.Depth = depth;
		Paint.WidthByte = (Paint.WidthMemory + 7) / 8;
	} else if (depth == 2) {
		Paint.Depth = depth;
		Paint.WidthByte = (Paint.WidthMemory + 3) / 4;
	} else if (depth == 4) {
		Paint.Depth = depth;
		Paint.WidthByte = (Paint.WidthMemory + 1) / 2;
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
