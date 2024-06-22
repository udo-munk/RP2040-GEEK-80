#include <string.h>
#include "pico/platform.h"
#include "GUI_Paint.h"
#include "Debug.h"

PAINT Paint;

/******************************************************************************
function: Create image
parameter:
    image   :   Pointer to the image cache
    width   :   The width of the picture
    Height  :   The height of the picture
    Color   :   Whether the picture is inverted
******************************************************************************/
void Paint_NewImage(uint8_t *image, uint16_t Width, uint16_t Height,
		    uint16_t Rotate, uint16_t Color)
{
	Paint.Image = NULL;
	Paint.Image = image;

	Paint.WidthMemory = Width;
	Paint.HeightMemory = Height;
	Paint.Color = Color;
	Paint.Depth = 16;

	Paint.WidthByte = Width * 2;
	Paint.HeightByte = Height;

	Paint.Rotate = Rotate;
	Paint.Mirror = MIRROR_NONE;

	if (Rotate == ROTATE_0 || Rotate == ROTATE_180) {
		Paint.Width = Width;
		Paint.Height = Height;
	} else {
		Paint.Width = Height;
		Paint.Height = Width;
	}
}

/******************************************************************************
function: Select image
parameter:
    image : Pointer to the image cache
******************************************************************************/
void Paint_SelectImage(uint8_t *image)
{
	Paint.Image = image;
}

/******************************************************************************
function: Select image rotate
parameter:
    Rotate : 0,90,180,270
******************************************************************************/
void Paint_SetRotate(uint16_t Rotate)
{
	if (Rotate == ROTATE_0 || Rotate == ROTATE_90 ||
	    Rotate == ROTATE_180 || Rotate == ROTATE_270) {
		Debug("Set image Rotate %d\r\n", Rotate);
		Paint.Rotate = Rotate;
	} else {
		Debug("rotate = 0, 90, 180, 270\r\n");
	}
}

/******************************************************************************
function: Select image depth
parameter:
    depth : 1, 2, 4, 8, 12, 16
******************************************************************************/
void Paint_SetDepth(uint8_t depth)
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

/******************************************************************************
function:	Select Image mirror
parameter:
    mirror   :Not mirror,Horizontal mirror,Vertical mirror,Origin mirror
******************************************************************************/
void Paint_SetMirroring(uint8_t mirror)
{
	if (mirror == MIRROR_NONE || mirror == MIRROR_HORIZONTAL ||
	    mirror == MIRROR_VERTICAL || mirror == MIRROR_ORIGIN) {
		Debug("mirror image x:%s, y:%s\r\n",
		      (mirror & 0x01) ? "mirror" : "none",
		      ((mirror >> 1) & 0x01) ? "mirror" : "none");
		Paint.Mirror = mirror;
	} else {
		Debug("mirror should be MIRROR_NONE, MIRROR_HORIZONTAL, "
		      "MIRROR_VERTICAL or MIRROR_ORIGIN\r\n");
	}
}

/******************************************************************************
function: Draw Pixels
parameter:
    Xpoint : At point X
    Ypoint : At point Y
    Color  : Painted colors
******************************************************************************/
void __time_critical_func(Paint_SetPixel)(uint16_t Xpoint, uint16_t Ypoint,
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
		Addr = ((X + 1) / 2) * 3 + Y * Paint.WidthByte;
		if ((X % 2) == 0) {
			Paint.Image[Addr] = (Color >> 4) & 0xff;
			Rdata = Paint.Image[Addr + 1] & 0x0f;
			Paint.Image[Addr + 1] = ((Color & 0x0f) << 4) | Rdata;
		} else {
			Rdata = Paint.Image[Addr - 2] & 0xf0;
			Paint.Image[Addr - 2] = Rdata | ((Color >> 8) & 0x0f);
			Paint.Image[Addr - 1] = Color & 0xff;
		}
	} else if (Paint.Depth == 16) {
		Addr = X * 2 + Y * Paint.WidthByte;
		Paint.Image[Addr] = (Color >> 8) & 0xff;
		Paint.Image[Addr + 1] = Color & 0xff;
	}
}

/******************************************************************************
function: Clear the color of the picture
parameter:
    Color : Painted colors
******************************************************************************/
void Paint_Clear(uint16_t Color)
{
	uint32_t Addr;
	uint16_t X, Y;
	int i;
	uint8_t Bytes[3];

	if (Paint.Depth == 1) {
		Color &= 0x1;
		Color |= Color << 1;
		Color |= Color << 2;
		Color |= Color << 4;
	} else if (Paint.Depth == 2) {
		Color &= 0x3;
		Color |= Color << 2;
		Color |= Color << 4;
	} else if (Paint.Depth == 4) {
		Color &= 0xf;
		Color |= Color << 4;
	}
	if (Paint.Depth <= 8 || Color == BLACK || Color == WHITE) {
		memset(Paint.Image, Color & 0xff,
		       (size_t) Paint.HeightByte * Paint.WidthByte);
	} else if (Paint.Depth == 12) {
		Bytes[0] = (Color >> 4) & 0xff;
		Bytes[1] = ((Color & 0xf) << 4) | ((Color >> 8) & 0xf);
		Bytes[2] = Color & 0xff;
		Addr = 0;
		i = 0;
		for (X = 0; X < Paint.WidthByte; X++) {
			Paint.Image[Addr++] = Bytes[i++];
			if (i == 3)
				i = 0;
		}
		for (Y = 1; Y < Paint.HeightByte; Y++) {
			memcpy(&Paint.Image[Addr], Paint.Image,
			       Paint.WidthByte);
			Addr += Paint.WidthByte;
		}
	} else if (Paint.Depth == 16) {
		Addr = 0;
		for (X = 0; X < Paint.WidthByte / 2; X++) {
			Paint.Image[Addr++] = (Color >> 8) & 0xff;
			Paint.Image[Addr++] = Color & 0xff;
		}
		for (Y = 1; Y < Paint.HeightByte; Y++) {
			memcpy(&Paint.Image[Addr], Paint.Image,
			       Paint.WidthByte);
			Addr += Paint.WidthByte;
		}
	}
}

/******************************************************************************
function: Clear the color of a window
parameter:
    Xstart : x starting point
    Ystart : Y starting point
    Xend   : x end point
    Yend   : y end point
    Color  : Painted colors
******************************************************************************/
void Paint_ClearWindows(uint16_t Xstart, uint16_t Ystart,
			uint16_t Xend, uint16_t Yend, uint16_t Color)
{
	uint16_t X, Y;

	for (Y = Ystart; Y < Yend; Y++)
		for (X = Xstart; X < Xend; X++)
			Paint_SetPixel(X, Y, Color);
}

/******************************************************************************
function: Draw Point(Xpoint, Ypoint) Fill the color
parameter:
    Xpoint		: The Xpoint coordinate of the point
    Ypoint		: The Ypoint coordinate of the point
    Color		: Painted color
    Dot_Pixel	: point size
    Dot_Style	: point Style
******************************************************************************/
void __time_critical_func(Paint_DrawPoint)(uint16_t Xpoint, uint16_t Ypoint,
					   uint16_t Color, DOT_PIXEL Dot_Pixel,
					   DOT_STYLE Dot_Style)
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
void __time_critical_func(Paint_DrawLine)(uint16_t Xstart, uint16_t Ystart,
					  uint16_t Xend, uint16_t Yend,
					  uint16_t Color, DOT_PIXEL Line_width,
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
void Paint_DrawRectangle(uint16_t Xstart, uint16_t Ystart,
			 uint16_t Xend, uint16_t Yend,
			 uint16_t Color, DOT_PIXEL Line_width,
			 DRAW_FILL Draw_Fill)
{
	uint16_t Ypoint;

	if (Xstart >= Paint.Width || Ystart >= Paint.Height ||
	    Xend >= Paint.Width || Yend >= Paint.Height) {
		Debug("Input exceeds the normal display range\r\n");
		return;
	}

	if (Draw_Fill) {
		for (Ypoint = Ystart; Ypoint < Yend; Ypoint++)
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

/******************************************************************************
function: Show English characters
parameter:
    Xpoint           : X coordinate
    Ypoint           : Y coordinate
    Acsii_Char       : To display the English characters
    Font             : A structure pointer that displays a character size
    Color_Foreground : Select the foreground color
    Color_Background : Select the background color
******************************************************************************/
void __time_critical_func(Paint_DrawChar)(uint16_t Xpoint, uint16_t Ypoint,
					  const char Acsii_Char,
					  sFONT *Font,
					  uint16_t Color_Foreground,
					  uint16_t Color_Background)
{
	uint16_t Page, Column;
	uint32_t Char_Offset;
	const unsigned char *ptr;

	if (Xpoint >= Paint.Width || Ypoint >= Paint.Height) {
		Debug("Paint_DrawChar Input exceeds the normal "
		      "display range\r\n");
		return;
	}

	Char_Offset = (Acsii_Char - ' ') * Font->Height *
		      (Font->Width / 8 + (Font->Width % 8 ? 1 : 0));
	ptr = &Font->table[Char_Offset];

	for (Page = 0; Page < Font->Height; Page++) {
		for (Column = 0; Column < Font->Width; Column++) {

			/* To determine whether the font background color and
			   screen background color is consistent */
			if (*ptr & (0x80 >> (Column % 8))) {
				Paint_SetPixel(Xpoint + Column, Ypoint + Page,
					       Color_Foreground);
				/* Paint_DrawPoint(Xpoint + Column,
						   Ypoint + Page,
						   Color_Foreground,
						   DOT_PIXEL_DFT,
						   DOT_STYLE_DFT); */
			} else {
				Paint_SetPixel(Xpoint + Column, Ypoint + Page,
					       Color_Background);
				/* Paint_DrawPoint(Xpoint + Column,
						   Ypoint + Page,
						   Color_Background,
						   DOT_PIXEL_DFT,
						   DOT_STYLE_DFT); */
			}
			/* One pixel is 8 bits */
			if (Column % 8 == 7)
				ptr++;
		} /* Write a line */
		if (Font->Width % 8 != 0)
			ptr++;
	} /* Write all */
}

/******************************************************************************
function:	Display the string
parameter:
    Xstart           : X coordinate
    Ystart           : Y coordinate
    pString          : The first address of the English string to be displayed
    Font             : A structure pointer that displays a character size
    Color_Foreground : Select the foreground color
    Color_Background : Select the background color
******************************************************************************/
void Paint_DrawString(uint16_t Xstart, uint16_t Ystart, const char *pString,
		      sFONT *Font, uint16_t Color_Foreground,
		      uint16_t Color_Background)
{
	uint16_t Xpoint = Xstart;
	uint16_t Ypoint = Ystart;

	if (Xstart >= Paint.Width || Ystart >= Paint.Height) {
		Debug("Paint_DrawString input exceeds the normal "
		      "display range\r\n");
		return;
	}

	while (*pString != '\0') {
		/* if X direction filled , reposition to(Xstart,Ypoint),
		   Ypoint is Y direction plus the Height of the character */
		if ((Xpoint + Font->Width) > Paint.Width) {
			Xpoint = Xstart;
			Ypoint += Font->Height;
		}

		/* If the Y direction is full,
		   reposition to (Xstart, Ystart) */
		if ((Ypoint + Font->Height) > Paint.Height) {
			Xpoint = Xstart;
			Ypoint = Ystart;
		}
		Paint_DrawChar(Xpoint, Ypoint, *pString, Font,
			       Color_Foreground, Color_Background);

		/* The next character of the address */
		pString++;

		/* The next word of the abscissa increases the font
		   of the broadband */
		Xpoint += Font->Width;
	}
}

/******************************************************************************
function:	Display nummber
parameter:
    Xstart           : X coordinate
    Ystart           : Y coordinate
    Nummber          : The number displayed
    Font             : A structure pointer that displays a character size
    Digit            : Fractional width
    Color_Foreground : Select the foreground color
    Color_Background : Select the background color
******************************************************************************/

#define ARRAY_LEN 255

void Paint_DrawNum(uint16_t Xpoint, uint16_t Ypoint, double Nummber,
		   sFONT *Font, uint16_t Digit, uint16_t Color_Foreground,
		   uint16_t Color_Background)
{
	char Str[ARRAY_LEN];

	if (Digit == 0)
		sprintf(Str, "%d", Nummber);
	else
		sprintf(Str, "%.*lf", Digit, Nummber);
	Paint_DrawString(Xpoint, Ypoint, (const char *) Str, Font,
			 Color_Foreground, Color_Background);
}

/******************************************************************************
function:	Display time
parameter:
    Xstart           : X coordinate
    Ystart           : Y coordinate
    pTime            : Time-related structures
    Font             : A structure pointer that displays a character size
    Color_Foreground : Select the foreground color
    Color_Background : Select the background color
******************************************************************************/
void Paint_DrawTime(uint16_t Xstart, uint16_t Ystart, PAINT_TIME *pTime,
		    sFONT *Font, uint16_t Color_Foreground,
		    uint16_t Color_Background)
{
	uint8_t value[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

	uint16_t Dx = Font->Width;

	/* Write data into the cache */
	Paint_DrawChar(Xstart, Ystart, value[pTime->Hour / 10], Font,
		       Color_Background, Color_Foreground);
	Paint_DrawChar(Xstart + Dx, Ystart, value[pTime->Hour % 10], Font,
		       Color_Background, Color_Foreground);
	Paint_DrawChar(Xstart + Dx + Dx / 4 + Dx / 2, Ystart, ':', Font,
		       Color_Background, Color_Foreground);
	Paint_DrawChar(Xstart + Dx * 2 + Dx / 2, Ystart,
		       value[pTime->Min / 10],
		       Font, Color_Background, Color_Foreground);
	Paint_DrawChar(Xstart + Dx * 3 + Dx / 2, Ystart,
		       value[pTime->Min % 10],
		       Font, Color_Background, Color_Foreground);
	Paint_DrawChar(Xstart + Dx * 4 + Dx / 2 - Dx / 4, Ystart, ':', Font,
		       Color_Background, Color_Foreground);
	Paint_DrawChar(Xstart + Dx * 5, Ystart, value[pTime->Sec / 10], Font,
		       Color_Background, Color_Foreground);
	Paint_DrawChar(Xstart + Dx * 6, Ystart, value[pTime->Sec % 10], Font,
		       Color_Background, Color_Foreground);
}

void Paint_DrawImage(const unsigned char *image,
		     uint16_t xStart, uint16_t yStart,
		     uint16_t W_Image, uint16_t H_Image)
{
	int i, j;

	for (j = 0; j < H_Image; j++) {
		for (i = 0; i < W_Image; i++) {
			/* Exceeded part does not display */
			if (xStart + i < Paint.WidthMemory &&
			    yStart + j < Paint.HeightMemory)
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

void Paint_DrawImage1(const unsigned char *image,
		      uint16_t xStart, uint16_t yStart,
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

/******************************************************************************
function:	Display monochrome bitmap
parameter:
    image_buffer : A picture data converted to a bitmap
info:
    Use a computer to convert the image into a corresponding array,
    and then embed the array directly into Imagedata.cpp as a .c file.
******************************************************************************/
void Paint_DrawBitMap(const unsigned char *image_buffer)
{
	uint16_t x, y;
	uint32_t Addr = 0;

	for (y = 0; y < Paint.HeightByte; y++) {
		for (x = 0; x < Paint.WidthByte; x++) {
			/* 8 pixel = 1 byte */
			Addr = x + y * Paint.WidthByte;
			Paint.Image[Addr] = (uint8_t) image_buffer[Addr];
		}
	}
}

void Paint_DrawBitMap_Block(const unsigned char *image_buffer, uint8_t Region)
{
	uint16_t x, y;
	uint32_t Addr = 0;

	for (y = 0; y < Paint.HeightByte; y++) {
		for (x = 0; x < Paint.WidthByte; x++) {
			/* 8 pixel = 1 byte */
			Addr = x + y * Paint.WidthByte;
			Paint.Image[Addr] =
				(uint8_t) image_buffer[Addr +
						       (Paint.HeightByte) *
						       Paint.WidthByte *
						       (Region - 1)];
		}
	}
}

void Paint_BmpWindows(unsigned char x, unsigned char y,
		      const unsigned char *pBmp,
		      unsigned char chWidth, unsigned char chHeight)
{
	uint16_t i, j, byteWidth = (chWidth + 7) / 8;

	for (j = 0; j < chHeight; j++)
		for (i = 0; i < chWidth; i++)
			if (*(pBmp + j * byteWidth + i / 8) &
			    (0x80 >> (i & 7)))
				Paint_SetPixel(x + i, y + j, 0xffff);
}
