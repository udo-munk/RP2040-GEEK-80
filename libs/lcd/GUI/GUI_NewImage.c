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
