#include "pico/platform.h"
#include "GUI_Paint.h"
#include "Debug.h"

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
void __not_in_flash_func(Paint_DrawChar)(uint16_t Xpoint, uint16_t Ypoint,
					 const char Acsii_Char,
					 const sFONT *Font,
					 uint16_t Color_Foreground,
					 uint16_t Color_Background)
{
	uint16_t Page, Column;
	uint32_t Char_Offset;
	uint8_t Char_Mask, Mask;
	const unsigned char *Char_Ptr, *ptr;

	if (Xpoint >= Paint.Width || Ypoint >= Paint.Height) {
		Debug("Paint_DrawChar Input exceeds the normal "
		      "display range\r\n");
		return;
	}

	Char_Offset = (Acsii_Char & 0x7f) * Font->Width;
	Char_Ptr = &Font->table[Char_Offset / 8];
	Char_Mask = 0x80 >> (Char_Offset % 8);

	for (Page = 0; Page < Font->Height; Page++) {
		Mask = Char_Mask;
		ptr = Char_Ptr;
		for (Column = 0; Column < Font->Width; Column++) {

			/* To determine whether the font background color and
			   screen background color is consistent */
			if (*ptr & Mask) {
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
			Mask = Mask >> 1;
			if (Mask == 0) {
				Mask = 0x80;
				ptr++;
			}
		} /* Write a line */
		Char_Ptr += Font->StripeWidth;
	} /* Write all */
}
