#include <stdio.h>
#include "pico/platform.h"
#include "GUI_Paint.h"
#include "Debug.h"

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
		    const sFONT *Font, uint16_t Color_Foreground,
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
