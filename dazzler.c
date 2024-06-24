/*
 * Emulation of Cromemco Dazzler on the RP2040-GEEK LCD
 *
 * Copyright (C) 2015-2019 by Udo Munk
 * Copyright (C) 2024 by Thomas Eberhardt
 */

#include <stdint.h>
#include "pico/platform.h"
#include "sim.h"
#include "simglb.h"
#include "config.h"
#include "memsim.h"
#include "dazzler.h"
#include "lcd.h"

/* Graphics stuff */
#if LCD_COLOR_DEPTH == 12
/* 444 colors and grays */
static const uint16_t __not_in_flash("color_map") colors[16] = {
	0x0000, 0x0800, 0x0080, 0x0880,
	0x0008, 0x0808, 0x0088, 0x0888,
	0x0000, 0x0f00, 0x00f0, 0x0ff0,
	0x000f, 0x0f0f, 0x00ff, 0x0fff
};
static const uint16_t __not_in_flash("color_map") grays[16] = {
	0x0000, 0x0111, 0x0222, 0x0333,
	0x0444, 0x0555, 0x0666, 0x0777,
	0x0888, 0x0999, 0x0aaa, 0x0bbb,
	0x0ccc, 0x0ddd, 0x0eee, 0x0fff
};
#else
/* 565 colors and grays */
static const uint16_t __not_in_flash("color_map") colors[16] = {
	0x0000, 0x8000, 0x0400, 0x8400,
	0x0010, 0x8010, 0x0410, 0x8410,
	0x0000, 0xf800, 0x07e0, 0xffe0,
	0x001f, 0xf81f, 0x07ff, 0xffff
};
static const uint16_t __not_in_flash("color_map") grays[16] = {
	0x0000, 0x1082, 0x2104, 0x31a6,
	0x4228, 0x52aa, 0x632c, 0x73ae,
	0x8c51, 0x9cd3, 0xad55, 0xbdd7,
	0xce59, 0xdefb, 0xef7d, 0xffff
};
#endif

#define CROMEMCO_W 17
#define CROMEMCO_H 132
static const uint8_t __not_in_flash("cromemco_logo") cromemco[] = {
	0x01, 0xf8, 0x00, 0x03, 0xfe, 0x00, 0x03, 0xff, 0x00, 0x03, 0xff, 0x00,
	0x07, 0x3f, 0x80, 0x06, 0x03, 0x80, 0x03, 0x01, 0x80, 0x03, 0xf1, 0x80,
	0x03, 0xff, 0x80, 0x01, 0xff, 0x80, 0x00, 0xff, 0x80, 0x00, 0x7f, 0x00,
	0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x03, 0xc2, 0x00,
	0x03, 0xe3, 0x00, 0x07, 0xc3, 0x00, 0x07, 0x03, 0x00, 0x07, 0x03, 0x80,
	0x03, 0x83, 0x80, 0x03, 0xff, 0x80, 0x01, 0xff, 0x80, 0x01, 0xff, 0x80,
	0x00, 0xff, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x03, 0xe3, 0x00, 0x07, 0xff, 0x00, 0x07, 0xff, 0x80,
	0x07, 0xff, 0x80, 0x03, 0xff, 0x80, 0x01, 0x00, 0x00, 0x01, 0x80, 0x00,
	0x03, 0xe0, 0x00, 0x07, 0xff, 0x00, 0x07, 0xff, 0x80, 0x07, 0xff, 0x80,
	0x03, 0xff, 0x80, 0x03, 0x87, 0x80, 0x01, 0x80, 0x00, 0x01, 0xc0, 0x00,
	0x07, 0xff, 0x00, 0x07, 0xff, 0x00, 0x03, 0xff, 0x80, 0x03, 0xff, 0x80,
	0x03, 0x0f, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x80, 0x00, 0x03, 0xc6, 0x00, 0x07, 0xe7, 0x00, 0x07, 0xf3, 0x00,
	0x07, 0xf3, 0x00, 0x06, 0x33, 0x80, 0x03, 0x1b, 0x80, 0x03, 0xff, 0x80,
	0x01, 0xff, 0x80, 0x01, 0xff, 0x00, 0x00, 0xff, 0x00, 0x00, 0x3e, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xe3, 0x00,
	0x07, 0xff, 0x00, 0x07, 0xff, 0x80, 0x07, 0xff, 0x80, 0x03, 0xff, 0x80,
	0x03, 0x03, 0x00, 0x01, 0x80, 0x00, 0x01, 0xc0, 0x00, 0x07, 0xff, 0x00,
	0x07, 0xff, 0x00, 0x07, 0xff, 0x80, 0x03, 0xff, 0x80, 0x03, 0x8f, 0x80,
	0x01, 0x80, 0x00, 0x01, 0xc0, 0x00, 0x07, 0xfe, 0x00, 0x07, 0xff, 0x00,
	0x07, 0xff, 0x80, 0x03, 0xff, 0x80, 0x03, 0x1f, 0x80, 0x01, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xf8, 0x00, 0x03, 0xfe, 0x00,
	0x07, 0xfe, 0x00, 0x07, 0xff, 0x00, 0x07, 0x3f, 0x00, 0x06, 0x03, 0x80,
	0x07, 0x01, 0x80, 0x03, 0xe3, 0x80, 0x03, 0xff, 0x80, 0x01, 0xff, 0x80,
	0x01, 0xff, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x1c, 0x00, 0x01, 0x00, 0x00,
	0x07, 0xc0, 0x00, 0x07, 0xc0, 0x00, 0x07, 0x80, 0x00, 0x03, 0x80, 0x00,
	0x01, 0xf0, 0x00, 0x07, 0xff, 0x00, 0x07, 0xff, 0x00, 0x07, 0xff, 0x80,
	0x03, 0xff, 0x80, 0x03, 0x07, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x3c, 0x18, 0x00, 0x7c, 0x1e, 0x00, 0x7c, 0x1e, 0x00,
	0x70, 0x0e, 0x00, 0xe0, 0x07, 0x00, 0xe0, 0x07, 0x00, 0xe0, 0x07, 0x80,
	0xf0, 0x07, 0x80, 0x7c, 0x0f, 0x80, 0x7f, 0xff, 0x80, 0x3f, 0xff, 0x80,
	0x3f, 0xff, 0x00, 0x1f, 0xff, 0x00, 0x07, 0xfe, 0x00, 0x03, 0xfc, 0x00
};

/* DAZZLER stuff */
static int state;
static WORD dma_addr;
static BYTE flags = 64;
static BYTE format;

/* centered image on 240x135 LCD */
#define XOFF	56
#define YOFF	3

/* pixel drawing inline functions */
static inline void pixel(uint16_t x, uint16_t y, uint16_t color)
{
	Paint_FastPixel(XOFF + x, YOFF + y, color);
}

static inline void pixel_2(uint16_t x, uint16_t y, uint16_t color)
{
	pixel(x * 2, y * 2, color);
	pixel(x * 2 + 1, y * 2, color);
	pixel(x * 2, y * 2 + 1, color);
	pixel(x * 2 + 1, y * 2 + 1, color);
}

static inline void pixel_4(uint16_t x, uint16_t y, uint16_t color)
{
	pixel_2(x * 2, y * 2, color);
	pixel_2(x * 2 + 1, y * 2, color);
	pixel_2(x * 2, y * 2 + 1, color);
	pixel_2(x * 2 + 1, y * 2 + 1, color);
}

/* draw pixels for one frame in hires */
static void __not_in_flash_func(draw_hires)(void)
{
	int x, y, i;
	WORD addr = dma_addr;
	uint16_t color;

	/* set color or grayscale from lower nibble in graphics format */
	i = format & 0x0f;
	color = (format & 16) ? colors[i] : grays[i];

	if (format & 32) {	/* 2048 bytes memory */
		for (y = 0; y < 64; y += 2) {
			for (x = 0; x < 64;) {
				i = dma_read(addr);
				pixel(x, y, (i & 1) ? color : BLACK);
				pixel(x + 1, y, (i & 2) ? color : BLACK);
				pixel(x, y + 1, (i & 4) ? color : BLACK);
				pixel(x + 1, y + 1, (i & 8) ? color : BLACK);
				pixel(x + 2, y, (i & 16) ? color : BLACK);
				pixel(x + 3, y, (i & 32) ? color : BLACK);
				pixel(x + 2, y + 1, (i & 64) ? color : BLACK);
				pixel(x + 3, y + 1, (i & 128) ? color : BLACK);
				x += 4;
				addr++;
			}
		}
		for (y = 0; y < 64; y += 2) {
			for (x = 64; x < 128;) {
				i = dma_read(addr);
				pixel(x, y, (i & 1) ? color : BLACK);
				pixel(x + 1, y, (i & 2) ? color : BLACK);
				pixel(x, y + 1, (i & 4) ? color : BLACK);
				pixel(x + 1, y + 1, (i & 8) ? color : BLACK);
				pixel(x + 2, y, (i & 16) ? color : BLACK);
				pixel(x + 3, y, (i & 32) ? color : BLACK);
				pixel(x + 2, y + 1, (i & 64) ? color : BLACK);
				pixel(x + 3, y + 1, (i & 128) ? color : BLACK);
				x += 4;
				addr++;
			}
		}
		for (y = 64; y < 128; y += 2) {
			for (x = 0; x < 64;) {
				i = dma_read(addr);
				pixel(x, y, (i & 1) ? color : BLACK);
				pixel(x + 1, y, (i & 2) ? color : BLACK);
				pixel(x, y + 1, (i & 4) ? color : BLACK);
				pixel(x + 1, y + 1, (i & 8) ? color : BLACK);
				pixel(x + 2, y, (i & 16) ? color : BLACK);
				pixel(x + 3, y, (i & 32) ? color : BLACK);
				pixel(x + 2, y + 1, (i & 64) ? color : BLACK);
				pixel(x + 3, y + 1, (i & 128) ? color : BLACK);
				x += 4;
				addr++;
			}
		}
		for (y = 64; y < 128; y += 2) {
			for (x = 64; x < 128;) {
				i = dma_read(addr);
				pixel(x, y, (i & 1) ? color : BLACK);
				pixel(x + 1, y, (i & 2) ? color : BLACK);
				pixel(x, y + 1, (i & 4) ? color : BLACK);
				pixel(x + 1, y + 1, (i & 8) ? color : BLACK);
				pixel(x + 2, y, (i & 16) ? color : BLACK);
				pixel(x + 3, y, (i & 32) ? color : BLACK);
				pixel(x + 2, y + 1, (i & 64) ? color : BLACK);
				pixel(x + 3, y + 1, (i & 128) ? color : BLACK);
				x += 4;
				addr++;
			}
		}
	} else {		/* 512 bytes memory */
		for (y = 0; y < 64; y += 2) {
			for (x = 0; x < 64;) {
				i = dma_read(addr);
				pixel_2(x, y, (i & 1) ? color : BLACK);
				pixel_2(x + 1, y, (i & 2) ? color : BLACK);
				pixel_2(x, y + 1, (i & 4) ? color : BLACK);
				pixel_2(x + 1, y + 1, (i & 8) ? color : BLACK);
				pixel_2(x + 2, y, (i & 16) ? color : BLACK);
				pixel_2(x + 3, y, (i & 32) ? color : BLACK);
				pixel_2(x + 2, y + 1, (i & 64) ? color : BLACK);
				pixel_2(x + 3, y + 1, (i & 128) ? color : BLACK);
				x += 4;
				addr++;
			}
		}
	}
}

/* draw pixels for one frame in lowres */
static void __not_in_flash_func(draw_lowres)(void)
{
	int x, y, i;
	WORD addr = dma_addr;
	uint16_t color;
	const uint16_t *cmap;

	cmap = (format & 16) ? colors : grays;
	/* get size of DMA memory and draw the pixels */
	if (format & 32) {	/* 2048 bytes memory */
		for (y = 0; y < 32; y++) {
			for (x = 0; x < 32;) {
				i = dma_read(addr) & 0x0f;
				color = cmap[i];
				pixel_2(x, y, color);
				x++;
				i = (dma_read(addr) & 0xf0) >> 4;
				color = cmap[i];
				pixel_2(x, y, color);
				x++;
				addr++;
			}
		}
		for (y = 0; y < 32; y++) {
			for (x = 32; x < 64;) {
				i = dma_read(addr) & 0x0f;
				color = cmap[i];
				pixel_2(x, y, color);
				x++;
				i = (dma_read(addr) & 0xf0) >> 4;
				color = cmap[i];
				pixel_2(x, y, color);
				x++;
				addr++;
			}
		}
		for (y = 32; y < 64; y++) {
			for (x = 0; x < 32;) {
				i = dma_read(addr) & 0x0f;
				color = cmap[i];
				pixel_2(x, y, color);
				x++;
				i = (dma_read(addr) & 0xf0) >> 4;
				color = cmap[i];
				pixel_2(x, y, color);
				x++;
				addr++;
			}
		}
		for (y = 32; y < 64; y++) {
			for (x = 32; x < 64;) {
				i = dma_read(addr) & 0x0f;
				color = cmap[i];
				pixel_2(x, y, color);
				x++;
				i = (dma_read(addr) & 0xf0) >> 4;
				color = cmap[i];
				pixel_2(x, y, color);
				x++;
				addr++;
			}
		}
	} else {		/* 512 bytes memory */
		for (y = 0; y < 32; y++) {
			for (x = 0; x < 32;) {
				i = dma_read(addr) & 0x0f;
				color = cmap[i];
				pixel_4(x, y, color);
				x++;
				i = (dma_read(addr) & 0xf0) >> 4;
				color = cmap[i];
				pixel_4(x, y, color);
				x++;
				addr++;
			}
		}
	}
}

static void __not_in_flash_func(dazzler_draw)(int first_flag)
{
	int i, j, bw;
	const uint8_t *p;

	if (first_flag) {
		Paint_Clear(BLACK);
		p = cromemco;
		bw = (CROMEMCO_W + 7) / 8;
		for (i = 0; i < CROMEMCO_H; i++) {
			for (j = 0; j < CROMEMCO_W; j++) {
				if (*(p + (j >> 3)) & (0x80 >> (j & 7)))
					Paint_FastPixel(10 + j, 1 + i, BRRED);
			}
			p += bw;
		}
		Paint_SetRotate(ROTATE_90);
		Paint_DrawString(18, 4, "DAZZLER", &Font28, BRRED, BLACK);
		Paint_SetRotate(ROTATE_0);
		return;
	}

	if (format & 64)
		draw_hires();
	else
		draw_lowres();

	/* frame done, set frame flag for 4ms */
	flags = 0;
	sleep_ms(4);
	flags = 64;
}

void dazzler_ctl_out(BYTE data)
{
	/* get DMA address for display memory */
	dma_addr = (data & 0x7f) << 9;

	/* switch DAZZLER on/off */
	if (data & 128) {
		if (state == 0) {
			state = 1;
			lcd_set_draw_func(dazzler_draw);
		}
	} else {
		if (state == 1) {
			state = 0;
			lcd_default_draw_func();
		}
	}
}

BYTE dazzler_flags_in(void)
{
	return flags;
}

void dazzler_format_out(BYTE data)
{
	format = data;
}
