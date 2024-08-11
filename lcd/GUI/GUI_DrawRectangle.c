#include "GUI_Paint.h"
#include "Debug.h"

/******************************************************************************
function: Draw a rectangle
parameter:
    Xstart : Rectangular  Starting Xpoint point coordinates
    Ystart : Rectangular  Starting Xpoint point coordinates
    Xend   : Rectangular  End point Xpoint coordinate
    Yend   : Rectangular  End point Ypoint coordinate
    Color  : The color of the Rectangular segment
    Line_width : Line width
    Draw_Fill  : Whether to fill the inside of the rectangle
******************************************************************************/
void Paint_DrawRectangle(uint16_t Xstart, uint16_t Ystart, uint16_t Xend,
			 uint16_t Yend, uint16_t Color, DOT_PIXEL Line_width,
			 DRAW_FILL Draw_Fill)
{
	uint16_t Ypoint;

	if (Xstart >= Paint.Width || Ystart >= Paint.Height ||
	    Xend >= Paint.Width || Yend >= Paint.Height) {
		Debug("Input exceeds the normal display range\r\n");
		return;
	}

	if (Draw_Fill) {
		for (Ypoint = Ystart; Ypoint <= Yend; Ypoint++)
			Paint_DrawLine(Xstart, Ypoint, Xend, Ypoint,
				       Color, Line_width, LINE_STYLE_SOLID);
	} else {
		Paint_DrawLine(Xstart, Ystart, Xend, Ystart,
			       Color, Line_width, LINE_STYLE_SOLID);
		Paint_DrawLine(Xstart, Ystart, Xstart, Yend,
			       Color, Line_width, LINE_STYLE_SOLID);
		Paint_DrawLine(Xend, Yend, Xend, Ystart,
			       Color, Line_width, LINE_STYLE_SOLID);
		Paint_DrawLine(Xend, Yend, Xstart, Yend,
			       Color, Line_width, LINE_STYLE_SOLID);
	}
}
