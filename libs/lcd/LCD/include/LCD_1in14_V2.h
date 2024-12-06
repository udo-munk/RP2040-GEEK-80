/*****************************************************************************
* | File	:   LCD_1IN14_V2.h
* | Function	:   test Demo
* | Info	:
*----------------
* | This version:   V1.0
* | Date	:   2021-03-16
* | Info	:
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
******************************************************************************/

#ifndef __LCD_1IN14_V2_H
#define __LCD_1IN14_V2_H

#include <stdint.h>

#define LCD_1IN14_V2_HEIGHT	240
#define LCD_1IN14_V2_WIDTH	135

#define LCD_HORIZONTAL		0
#define LCD_VERTICAL		1

typedef struct {
	uint16_t WIDTH;
	uint16_t HEIGHT;
	uint8_t SCAN_DIR;
	uint8_t ROTATED;
} LCD_1IN14_V2_ATTRIBUTES;

extern LCD_1IN14_V2_ATTRIBUTES LCD_1IN14_V2;

extern void LCD_1IN14_V2_Init(uint8_t Scan_dir);
extern void LCD_1IN14_V2_Exit(void);
extern void LCD_1IN14_V2_SetRotated(uint8_t Rotated);
extern void LCD_1IN14_V2_Clear(uint16_t Color);
extern void LCD_1IN14_V2_Display(uint16_t *Image);
extern void LCD_1IN14_V2_Display12(uint8_t *Image);
extern void LCD_1IN14_V2_DisplayWindows(uint16_t Xstart, uint16_t Ystart,
					uint16_t Xend, uint16_t Yend,
					uint16_t *Image);
extern void LCD_1IN14_V2_DisplayPoint(uint16_t X, uint16_t Y, uint16_t Color);
extern void LCD_1IN14_V2_SetBacklight(uint8_t Value);

#endif
