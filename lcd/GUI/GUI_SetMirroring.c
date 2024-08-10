#include "GUI_Paint.h"
#include "Debug.h"

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
