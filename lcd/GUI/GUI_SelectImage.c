#include "pico/platform.h"
#include "GUI_Paint.h"
#include "Debug.h"

/******************************************************************************
function: Select image
parameter:
    image : Pointer to the image cache
******************************************************************************/
void __not_in_flash_func(Paint_SelectImage)(uint8_t *image)
{
	Paint.Image = image;
}
