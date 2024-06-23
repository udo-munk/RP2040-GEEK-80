/*
 * Emulation of Cromemco Dazzler on the RP2040-GEEK LCD
 *
 * Copyright (C) 2024 by Udo Munk & Thomas Eberhardt
 */

#include <stdint.h>
#include "sim.h"
#include "simglb.h"
#include "dazzler.h"
#include "lcd.h"

#if LCD_COLOR_DEPTH == 12

#else
#endif

volatile BYTE dazzler_flags = 64;
volatile int dazzler_on;

static WORD dma_addr;
static BYTE format;

static void dazzler_draw(int);

void dazzler_ctl_out(BYTE data)
{
	/* get DMA address for display memory */
	dma_addr = (data & 0x7f) << 9;

	/* switch DAZZLER on/off */
	if (data & 128) {
		if (dazzler_on == 0) {
			lcd_set_draw_func(dazzler_draw);
			dazzler_on = 1;
		}
	} else {
		if (dazzler_on != 0) {
			lcd_default_draw_func();
			dazzler_on = 0;
		}
	}
}

void dazzler_format_out(BYTE data)
{
	format = data;
}

BYTE dazzler_flags_in(void)
{
	return dazzler_flags;
}

static void dazzler_draw(int first_flag)
{
	if (first_flag) {
		Paint_Clear(BLACK);
	} else {
	}
}
