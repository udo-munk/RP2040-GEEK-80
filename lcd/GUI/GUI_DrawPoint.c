#include "pico/platform.h"
#include "GUI_Paint.h"
#include "Debug.h"

/******************************************************************************
function: Draw Point(Xpoint, Ypoint) Fill the color
parameter:
    Xpoint		: The Xpoint coordinate of the point
    Ypoint		: The Ypoint coordinate of the point
    Color		: Painted color
    Dot_Pixel	: point size
    Dot_Style	: point Style
******************************************************************************/
void Paint_DrawPoint(uint16_t Xpoint, uint16_t Ypoint, uint16_t Color,
		     DOT_PIXEL Dot_Pixel, DOT_STYLE Dot_Style)
{
	int16_t XDir_Num, YDir_Num;

	if (Xpoint >= Paint.Width || Ypoint >= Paint.Height) {
		Debug("Paint_DrawPoint Input exceeds the normal "
		      "display range\r\n");
		return;
	}

	if (Dot_Style == DOT_FILL_AROUND) {
		for (XDir_Num = 0; XDir_Num < 2 * Dot_Pixel - 1; XDir_Num++)
			for (YDir_Num = 0; YDir_Num < 2 * Dot_Pixel - 1;
			     YDir_Num++) {
				if (Xpoint + XDir_Num - Dot_Pixel < 0 ||
				    Ypoint + YDir_Num - Dot_Pixel < 0)
					break;
				Paint_SetPixel(Xpoint + XDir_Num - Dot_Pixel,
					       Ypoint + YDir_Num - Dot_Pixel,
					       Color);
			}
	} else {
		for (XDir_Num = 0; XDir_Num < Dot_Pixel; XDir_Num++)
			for (YDir_Num = 0; YDir_Num < Dot_Pixel; YDir_Num++)
				Paint_SetPixel(Xpoint + XDir_Num - 1,
					       Ypoint + YDir_Num - 1,
					       Color);
	}
}
