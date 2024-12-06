#include "GUI_Paint.h"
#include "Debug.h"

/******************************************************************************
function: Select image
parameter:
    image : Pointer to the image cache
******************************************************************************/
void Paint_SelectImage(uint8_t *image)
{
	Paint.Image = image;
}
