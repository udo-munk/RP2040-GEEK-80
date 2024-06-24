#include "pico/platform.h"
#include "GUI_Paint.h"
#include "Debug.h"

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
void __not_in_flash_func(Paint_DrawString)(uint16_t Xstart, uint16_t Ystart,
					   const char *pString,
					   const sFONT *Font,
					   uint16_t Color_Foreground,
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
