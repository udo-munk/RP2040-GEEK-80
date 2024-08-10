#include "GUI_Paint.h"
#include "Debug.h"

/******************************************************************************
function: Clear the color of a window
parameter:
    Xstart : x starting point
    Ystart : Y starting point
    Xend   : x end point
    Yend   : y end point
    Color  : Painted colors
******************************************************************************/
void Paint_ClearWindow(uint16_t Xstart, uint16_t Ystart, uint16_t Xend,
		       uint16_t Yend, uint16_t Color)
{
	uint16_t X, Y;

	for (Y = Ystart; Y < Yend; Y++)
		for (X = Xstart; X < Xend; X++)
			Paint_SetPixel(X, Y, Color);
}
