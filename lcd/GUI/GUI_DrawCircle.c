#include "pico/platform.h"
#include "GUI_Paint.h"
#include "Debug.h"

/******************************************************************************
function: Use the 8-point method to draw a circle of the
            specified size at the specified position->
parameter:
    X_Center  : Center X coordinate
    Y_Center  : Center Y coordinate
    Radius    : circle Radius
    Color     : The color of the : circle segment
    Line_width: Line width
    Draw_Fill : Whether to fill the inside of the Circle
******************************************************************************/
void Paint_DrawCircle(uint16_t X_Center, uint16_t Y_Center, uint16_t Radius,
		      uint16_t Color, DOT_PIXEL Line_width,
		      DRAW_FILL Draw_Fill)
{
	int16_t XCurrent, YCurrent, Esp, sCountY;

	if (X_Center >= Paint.Width || Y_Center >= Paint.Height) {
		Debug("Paint_DrawCircle Input exceeds the normal "
		      "display range\r\n");
		return;
	}

	// Draw a circle from(0, R) as a starting point
	XCurrent = 0;
	YCurrent = Radius;

	// Cumulative error,judge the next point of the logo
	Esp = 3 - (Radius << 1);

	if (Draw_Fill == DRAW_FILL_FULL) {
		while (XCurrent <= YCurrent) {
			/* Realistic circles */
			for (sCountY = XCurrent; sCountY <= YCurrent;
			     sCountY++) {
				Paint_DrawPoint(X_Center + XCurrent,
						Y_Center + sCountY,
						Color, DOT_PIXEL_DFT,
						DOT_STYLE_DFT); /* 1 */
				Paint_DrawPoint(X_Center - XCurrent,
						Y_Center + sCountY,
						Color, DOT_PIXEL_DFT,
						DOT_STYLE_DFT); /* 2 */
				Paint_DrawPoint(X_Center - sCountY,
						Y_Center + XCurrent,
						Color, DOT_PIXEL_DFT,
						DOT_STYLE_DFT); /* 3 */
				Paint_DrawPoint(X_Center - sCountY,
						Y_Center - XCurrent,
						Color, DOT_PIXEL_DFT,
						DOT_STYLE_DFT); /* 4 */
				Paint_DrawPoint(X_Center - XCurrent,
						Y_Center - sCountY,
						Color, DOT_PIXEL_DFT,
						DOT_STYLE_DFT); /* 5 */
				Paint_DrawPoint(X_Center + XCurrent,
						Y_Center - sCountY,
						Color, DOT_PIXEL_DFT,
						DOT_STYLE_DFT); /* 6 */
				Paint_DrawPoint(X_Center + sCountY,
						Y_Center - XCurrent,
						Color, DOT_PIXEL_DFT,
						DOT_STYLE_DFT); /* 7 */
				Paint_DrawPoint(X_Center + sCountY,
						Y_Center + XCurrent,
						Color, DOT_PIXEL_DFT,
						DOT_STYLE_DFT); /* 0 */
			}
			if (Esp < 0)
				Esp += 4 * XCurrent + 6;
			else {
				Esp += 10 + 4 * (XCurrent - YCurrent);
				YCurrent--;
			}
			XCurrent++;
		}
	} else {
		/* Draw a hollow circle */
		while (XCurrent <= YCurrent) {
			Paint_DrawPoint(X_Center + XCurrent,
					Y_Center + YCurrent, Color,
					Line_width, DOT_STYLE_DFT); /* 1 */
			Paint_DrawPoint(X_Center - XCurrent,
					Y_Center + YCurrent, Color,
					Line_width, DOT_STYLE_DFT); /* 2 */
			Paint_DrawPoint(X_Center - YCurrent,
					Y_Center + XCurrent, Color,
					Line_width, DOT_STYLE_DFT); /* 3 */
			Paint_DrawPoint(X_Center - YCurrent,
					Y_Center - XCurrent, Color,
					Line_width, DOT_STYLE_DFT); /* 4 */
			Paint_DrawPoint(X_Center - XCurrent,
					Y_Center - YCurrent, Color,
					Line_width, DOT_STYLE_DFT); /* 5 */
			Paint_DrawPoint(X_Center + XCurrent,
					Y_Center - YCurrent, Color,
					Line_width, DOT_STYLE_DFT); /* 6 */
			Paint_DrawPoint(X_Center + YCurrent,
					Y_Center - XCurrent, Color,
					Line_width, DOT_STYLE_DFT); /* 7 */
			Paint_DrawPoint(X_Center + YCurrent,
					Y_Center + XCurrent, Color,
					Line_width, DOT_STYLE_DFT); /* 0 */

			if (Esp < 0)
				Esp += 4 * XCurrent + 6;
			else {
				Esp += 10 + 4 * (XCurrent - YCurrent);
				YCurrent--;
			}
			XCurrent++;
		}
	}
}
