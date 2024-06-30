#include "pico/platform.h"
#include "GUI_Paint.h"
#include "Debug.h"

/******************************************************************************
function: Draw a line of arbitrary slope
parameter:
    Xstart : Starting Xpoint point coordinates
    Ystart : Starting Xpoint point coordinates
    Xend   : End point Xpoint coordinate
    Yend   : End point Ypoint coordinate
    Color  : The color of the line segment
    Line_width : Line width
    Line_Style : Solid and dotted lines
******************************************************************************/
void Paint_DrawLine(uint16_t Xstart, uint16_t Ystart, uint16_t Xend,
		    uint16_t Yend, uint16_t Color, DOT_PIXEL Line_width,
		    LINE_STYLE Line_Style)
{
	uint16_t Xpoint, Ypoint;
	int dx, dy, XAddway, YAddway, Esp;
	char Dotted_Len;

	if (Xstart >= Paint.Width || Ystart >= Paint.Height ||
	    Xend >= Paint.Width || Yend >= Paint.Height) {
		Debug("Paint_DrawLine Input exceeds the normal "
		      "display range\r\n");
		return;
	}

	Xpoint = Xstart;
	Ypoint = Ystart;
	dx = (int) Xend - (int) Xstart >= 0 ? Xend - Xstart : Xstart - Xend;
	dy = (int) Yend - (int) Ystart <= 0 ? Yend - Ystart : Ystart - Yend;

	/* Increment direction, 1 is positive, -1 is counter */
	XAddway = Xstart < Xend ? 1 : -1;
	YAddway = Ystart < Yend ? 1 : -1;

	/* Cumulative error */
	Esp = dx + dy;
	Dotted_Len = 0;

	for (;;) {
		Dotted_Len++;
		/* Painted dotted line, 2 point is really virtual */
		if (Line_Style == LINE_STYLE_DOTTED && Dotted_Len % 3 == 0) {
			/* Debug("LINE_DOTTED\r\n"); */
			if (Color)
				Paint_DrawPoint(Xpoint, Ypoint, BLACK,
						Line_width, DOT_STYLE_DFT);
			else
				Paint_DrawPoint(Xpoint, Ypoint, WHITE,
						Line_width, DOT_STYLE_DFT);
			Dotted_Len = 0;
		} else {
			Paint_DrawPoint(Xpoint, Ypoint, Color,
					Line_width, DOT_STYLE_DFT);
		}
		if (2 * Esp >= dy) {
			if (Xpoint == Xend)
				break;
			Esp += dy;
			Xpoint += XAddway;
		}
		if (2 * Esp <= dx) {
			if (Ypoint == Yend)
				break;
			Esp += dx;
			Ypoint += YAddway;
		}
	}
}
