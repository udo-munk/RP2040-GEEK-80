#include "GUI_Paint.h"
#include "Debug.h"

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
