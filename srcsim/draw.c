/*
 * Functions for drawing into a pixmap (supports COLOR_DEPTH 12 and 16)
 *
 * Copyright (C) 2024 by Thomas Eberhardt
 */

#include <string.h>

#include "pico.h"

#include "draw.h"

draw_pixmap_t *draw_pixmap;	/* active pixmap */

/*
 *	Fill the pixmap with the specified color.
 */
void __not_in_flash_func(draw_clear)(uint16_t color)
{
	uint8_t *p = draw_pixmap->bits;
	uint16_t x, y;

#if COLOR_DEPTH == 12
	int i;
	uint8_t pixel_pair[3];

	pixel_pair[0] = (color >> 4) & 0xff;
	pixel_pair[1] = ((color & 0xf) << 4) | ((color >> 8) & 0xf);
	pixel_pair[2] = color & 0xff;
	i = 0;
	for (x = 0; x < draw_pixmap->stride; x++) {
		*p++ = pixel_pair[i++];
		if (i == 3)
			i = 0;
	}
#else
	for (x = 0; x < draw_pixmap->width; x++) {
		*p++ = (color >> 8) & 0xff;
		*p++ = color & 0xff;
	}
#endif
	for (y = 1; y < draw_pixmap->height; y++) {
		memcpy(p, draw_pixmap->bits, draw_pixmap->stride);
		p += draw_pixmap->stride;
	}
}

/*
 *	Draw a string using the specified font and colors.
 */
void __not_in_flash_func(draw_string)(uint16_t x, uint16_t y, const char *s,
				      const font_t *font, uint16_t fgc,
				      uint16_t bgc)
{
	while (*s) {
		draw_char(x, y, *s++, font, fgc, bgc);
		x += font->width;
	}
}

/*
 *	Draw a bitmap in the specified color (always uses a depth of 1).
 */
void __not_in_flash_func(draw_bitmap)(uint16_t x, uint16_t y,
				      const draw_ro_pixmap_t *bitmap,
				      uint16_t color)
{
	const uint8_t *p0 = bitmap->bits, *p;
	uint8_t m;
	uint16_t i, j;

	for (j = bitmap->height; j > 0; j--) {
		m = 0x80;
		p = p0;
		for (i = bitmap->width; i > 0; i--) {
			if (*p & m)
				draw_pixel(x, y, color);
			if ((m >>= 1) == 0) {
				m = 0x80;
				p++;
			}
			x++;
		}
		x -= bitmap->width;
		y++;
		p0 += bitmap->stride;
	}
}

/*
 *	Draw centered framed banner text
 */
void draw_banner(const draw_banner_t *banner, const font_t *font,
		 uint16_t color)
{
	int i, lines;
	uint16_t y0;
	const draw_banner_t *bp;

	lines = 0;
	for (bp = banner; bp->text; bp++)
		lines++;
	draw_clear(C_BLACK);
	draw_hline(0, 0, draw_pixmap->width, color);
	draw_vline(0, 0, draw_pixmap->height, color);
	draw_vline(draw_pixmap->width - 1, 0, draw_pixmap->height, color);
	draw_hline(0, draw_pixmap->height - 1, draw_pixmap->width, color);
	y0 = (draw_pixmap->height - lines * (font->height + 2)) / 2;
	for (i = 0; i < lines; i++) {
		draw_string((draw_pixmap->width - strlen(banner->text) *
			     font->width) / 2,
			    y0 + i * (font->height + 2), banner->text, font,
			    banner->color, C_BLACK);
		banner++;
	}
}
