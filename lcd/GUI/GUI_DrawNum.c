#include <stdio.h>

#include "GUI_Paint.h"
#include "Debug.h"

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
		   const sFONT *Font, uint16_t Digit,
		   uint16_t Color_Foreground, uint16_t Color_Background)
{
	char Str[ARRAY_LEN];

	if (Digit == 0)
		sprintf(Str, "%d", Nummber);
	else
		sprintf(Str, "%.*lf", Digit, Nummber);
	Paint_DrawString(Xpoint, Ypoint, (const char *) Str, Font,
			 Color_Foreground, Color_Background);
}
