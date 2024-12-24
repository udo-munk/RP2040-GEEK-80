/*
 * Functions for drawing into a pixmap (supports COLOR_DEPTH 12 and 16)
 *
 * Copyright (C) 2024 by Thomas Eberhardt
 */

#ifndef DRAW_INC
#define DRAW_INC

#include <stdint.h>
#ifdef DRAW_DEBUG
#include <stdio.h>
#endif

#if COLOR_DEPTH != 12 && COLOR_DEPTH != 16
#error "Unsupported COLOR_DEPTH"
#endif

/*
 *	Colors
 */
#if COLOR_DEPTH == 12
/* 444 RGB colors */
#define C_BLACK		0x0000
#define C_RED		0x0f00
#define C_GREEN		0x00f0
#define C_BLUE		0x000f
#define C_CYAN		0x00ff
#define C_MAGENTA	0x0f0f
#define C_YELLOW	0x0ff0
#define C_WHITE		0x0fff
#define C_DKRED		0x0800
#define C_DKGREEN	0x0080
#define C_DKBLUE	0x0008
#define C_DKCYAN	0x0088
#define C_DKMAGENTA	0x0808
#define C_DKYELLOW	0x0880
#define C_GRAY		0x0888
#define C_ORANGE	0x0fa0
#else
/* 565 RGB colors */
#define C_BLACK		0x0000
#define C_RED		0xf800
#define C_GREEN		0x07e0
#define C_BLUE		0x001f
#define C_CYAN		0x7fff
#define C_MAGENTA	0xf81f
#define C_YELLOW	0xffe0
#define C_WHITE		0xffff
#define C_DKRED		0x8800
#define C_DKGREEN	0x0440
#define C_DKBLUE	0x0011
#define C_DKCYAN	0x0451
#define C_DKMAGENTA	0x8811
#define C_DKYELLOW	0x4c40
#define C_GRAY		0x8410
#define C_ORANGE	0xfd20
#endif

/*
 *	Pixmap type for drawing into.
 *	The depth field is currently ignored, and COLOR_DEPTH is used for
 *	conditional compilation.
 */
typedef struct draw_pixmap {
	uint8_t *bits;
	uint8_t depth;
	uint16_t width;
	uint16_t height;
	uint16_t stride;
} draw_pixmap_t;

/*
 *	Pixmap type used as read-only source (fonts, prepared images).
 *	In fonts the width field specifies the width of a single character.
 */
typedef const struct draw_ro_pixmap {
	const uint8_t *bits;
	const uint8_t depth;
	const uint16_t width;
	const uint16_t height;
	const uint16_t stride;
} draw_ro_pixmap_t;
typedef draw_ro_pixmap_t font_t;

/*
 *	Element type of NULL terminated array drawn by draw_banner().
 */
typedef struct draw_banner {
	const char *text;
	uint16_t color;
} draw_banner_t;

/*
 *	Grid type for drawing text with character based coordinates.
 */
typedef struct draw_grid {
	const font_t *font;
	uint16_t xoff;
	uint16_t yoff;
	uint16_t spc;
	uint16_t cwidth;
	uint16_t cheight;
	uint16_t cols;
	uint16_t rows;
} draw_grid_t;

extern draw_pixmap_t *draw_pixmap;

/*
 *	Declaration of available fonts.
 */
extern const font_t font12;	/* 6 x 12 pixels */
extern const font_t font14;	/* 8 x 14 pixels */
extern const font_t font16;	/* 8 x 16 pixels */
extern const font_t font18;	/* 10 x 18 pixels */
extern const font_t font20;	/* 10 x 20 pixels */
extern const font_t font22;	/* 11 x 22 pixels */
extern const font_t font24;	/* 12 x 24 pixels */
extern const font_t font28;	/* 14 x 28 pixels */
extern const font_t font32;	/* 16 x 32 pixels */

extern void draw_clear(uint16_t color);
extern void draw_string(uint16_t x, uint16_t y, const char *s,
			const font_t *font, uint16_t fgc, uint16_t bgc);
extern void draw_bitmap(uint16_t x, uint16_t y, const draw_ro_pixmap_t *bitmap,
			uint16_t color);
extern void draw_banner(const draw_banner_t *banner, const font_t *font,
			uint16_t color);

/*
 *	Set active pixmap
 */
static inline void draw_set_pixmap(draw_pixmap_t *pixmap)
{
#ifdef DRAW_DEBUG
	if (pixmap == NULL)
		fprintf(stderr, "%s: NULL pixmap specified\n", __func__);
#endif
	draw_pixmap = pixmap;
}

/*
 *	Draw a pixel in the specified color.
 */
static inline void draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
	uint8_t *p;

#ifdef DRAW_DEBUG
	if (draw_pixmap == NULL) {
		fprintf(stderr, "%s: draw pixmap is NULL\n", __func__);
		return;
	}
	if (x >= draw_pixmap->width || y >= draw_pixmap->height) {
		fprintf(stderr, "%s: coord (%d,%d) is outside (0,0)-(%d,%d)\n",
			__func__, x, y, draw_pixmap->width - 1,
			draw_pixmap->height - 1);
		return;
	}
#endif
#if COLOR_DEPTH == 12
	p = draw_pixmap->bits + ((x >> 1) * 3 + y * draw_pixmap->stride);
	if ((x & 1) == 0) {
		*p++ = (color >> 4) & 0xff;
		*p = ((color & 0x0f) << 4) | (*p & 0x0f);
	} else {
		p++;
		*p = (*p & 0xf0) | ((color >> 8) & 0x0f);
		*++p = color & 0xff;
	}
#else
	p = draw_pixmap->bits + ((x << 1) + y * draw_pixmap->stride);
	*p++ = (color >> 8) & 0xff;
	*p = color & 0xff;
#endif
}

/*
 *	Draw a character in the specfied font and colors.
 */
static inline void draw_char(uint16_t x, uint16_t y, const char c,
			     const font_t *font, uint16_t fgc, uint16_t bgc)
{
	const uint16_t off = (c & 0x7f) * font->width;
	const uint8_t *p0 = font->bits + (off >> 3), *p;
	const uint8_t m0 = 0x80 >> (off & 7);
	uint8_t m;
	uint16_t i, j;

#ifdef DRAW_DEBUG
	if (draw_pixmap == NULL) {
		fprintf(stderr, "%s: draw pixmap is NULL\n", __func__);
		return;
	}
	if (font == NULL) {
		fprintf(stderr, "%s: font is NULL\n", __func__);
		return;
	}
	if (x >= draw_pixmap->width || y >= draw_pixmap->height ||
	    x + font->width > draw_pixmap->width ||
	    y + font->height > draw_pixmap->height) {
		fprintf(stderr, "%s: char '%c' at (%d,%d)-(%d,%d) is "
			"outside (0,0)-(%d,%d)\n", __func__, c, x, y,
			x + font->width - 1, y + font->height - 1,
			draw_pixmap->width - 1, draw_pixmap->height - 1);
		return;
	}
#endif
	for (j = font->height; j > 0; j--) {
		m = m0;
		p = p0;
		for (i = font->width; i > 0; i--) {
			if (*p & m)
				draw_pixel(x, y, fgc);
			else
				draw_pixel(x, y, bgc);
			if ((m >>= 1) == 0) {
				m = 0x80;
				p++;
			}
			x++;
		}
		x -= font->width;
		y++;
		p0 += font->stride;
	}
}

/*
 *	Draw a horizontal line in the specified color.
 */
static inline void draw_hline(uint16_t x, uint16_t y, uint16_t w, uint16_t col)
{
#ifdef DRAW_DEBUG
	if (draw_pixmap == NULL) {
		fprintf(stderr, "%s: draw pixmap is NULL\n", __func__);
		return;
	}
	if (x >= draw_pixmap->width || y >= draw_pixmap->height ||
	    x + w > draw_pixmap->width) {
		fprintf(stderr, "%s: line (%d,%d)-(%d,%d) is outside "
			"(0,0)-(%d,%d)\n", __func__, x, y, x + w - 1, y,
			draw_pixmap->width - 1, draw_pixmap->height - 1);
		return;
	}
#endif
	while (w--)
		draw_pixel(x++, y, col);
}

/*
 *	Draw a vertical line in the specified color.
 */
static inline void draw_vline(uint16_t x, uint16_t y, uint16_t h, uint16_t col)
{
#ifdef DRAW_DEBUG
	if (draw_pixmap == NULL) {
		fprintf(stderr, "%s: draw pixmap is NULL\n", __func__);
		return;
	}
	if (x >= draw_pixmap->width || y >= draw_pixmap->height ||
	    y + h > draw_pixmap->width) {
		fprintf(stderr, "%s: line (%d,%d)-(%d,%d) is outside "
			"(0,0)-(%d,%d)\n", __func__, x, y, x, y + h - 1,
			draw_pixmap->width - 1, draw_pixmap->height - 1);
		return;
	}
#endif
	while (h--)
		draw_pixel(x, y++, col);
}

/*
 *	Setup a text grid defined by font and spacing.
 *	If col < 0 then use the entire draw pixmap width.
 *	If row < 0 then use the entire draw pixmap height.
 */
static inline void draw_setup_grid(draw_grid_t *grid, uint16_t xoff,
				   uint16_t yoff, int cols, int rows,
				   const font_t *font, uint16_t spc)
{
#ifdef DRAW_DEBUG
	if (draw_pixmap == NULL) {
		fprintf(stderr, "%s: draw pixmap is NULL\n", __func__);
		return;
	}
	if (grid == NULL) {
		fprintf(stderr, "%s: grid is NULL\n", __func__);
		return;
	}
	if (font == NULL) {
		fprintf(stderr, "%s: font is NULL\n", __func__);
		return;
	}
	if (cols == 0) {
		fprintf(stderr," %s: number of columns is zero\n", __func__);
		return;
	}
	if (cols > (draw_pixmap->width - xoff) / grid->cwidth) {
		fprintf(stderr," %s: number of columns %d is too large\n",
			__func__, cols);
		return;
	}
	if (rows == 0) {
		fprintf(stderr," %s: number of rows is zero\n", __func__);
		return;
	}
	if (rows > (draw_pixmap->height - yoff + spc) / grid->cheight) {
		fprintf(stderr," %s: number of rows %d is too large\n",
			__func__, rows);
		return;
	}
#endif
	grid->font = font;
	grid->xoff = xoff;
	grid->yoff = yoff;
	grid->spc = spc;
	grid->cwidth = font->width;
	grid->cheight = font->height + spc;
	if (cols < 0)
		grid->cols = (draw_pixmap->width - xoff) / grid->cwidth;
	else
		grid->cols = cols;
	if (rows < 0)
		grid->rows = (draw_pixmap->height - yoff + spc) /
			grid->cheight;
	else
		grid->rows = rows;
}

/*
 *	Draw a character using grid coordinates in the specified color.
 */
static inline void draw_grid_char(uint16_t x, uint16_t y, char c,
				  const draw_grid_t *grid, uint16_t fgc,
				  uint16_t bgc)
{
#ifdef DRAW_DEBUG
	if (draw_pixmap == NULL) {
		fprintf(stderr, "%s: draw pixmap is NULL\n", __func__);
		return;
	}
	if (grid == NULL) {
		fprintf(stderr, "%s: grid is NULL\n", __func__);
		return;
	}
#endif
	draw_char(x * grid->cwidth + grid->xoff,
		  y * grid->cheight + grid->yoff,
		  c, grid->font, fgc, bgc);
}

/*
 *	Draw a horizontal grid line in the middle of the spacing
 *	above the y grid coordinate specified.
 */
static inline void draw_grid_hline(uint16_t x, uint16_t y, uint16_t w,
				   const draw_grid_t *grid, uint16_t col)
{
#ifdef DRAW_DEBUG
	if (draw_pixmap == NULL) {
		fprintf(stderr, "%s: draw pixmap is NULL\n", __func__);
		return;
	}
	if (grid == NULL) {
		fprintf(stderr, "%s: grid is NULL\n", __func__);
		return;
	}
#endif
	if (w) {
		x = x * grid->cwidth;
		if (y)
			y = y * grid->cheight - (grid->spc + 1) / 2;
		w = w * grid->cwidth;
		draw_hline(x + grid->xoff, y + grid->yoff, w, col);
	}
}

/*
 *	Draw a vertical grid line in the middle of the x grid coordinate
 *	specified.
 */
static inline void draw_grid_vline(uint16_t x, uint16_t y, uint16_t h,
				   const draw_grid_t *grid, uint16_t col)
{
#ifdef DRAW_DEBUG
	if (draw_pixmap == NULL) {
		fprintf(stderr, "%s: draw pixmap is NULL\n", __func__);
		return;
	}
	if (grid == NULL) {
		fprintf(stderr, "%s: grid is NULL\n", __func__);
		return;
	}
#endif
	uint16_t hadj = 0;

	if (h) {
		x = x * grid->cwidth + (grid->cwidth + 1) / 2;
		if (y + h < grid->rows)
			hadj += grid->spc / 2 + 1;
		if (y) {
			y = y * grid->cheight - (grid->spc + 1) / 2;
			hadj += (grid->spc + 1) / 2;
		}
		h = h * grid->cheight - grid->spc + hadj;
		draw_vline(x + grid->xoff, y + grid->yoff, h, col);
	}
}

/*
 *	Draw a 10x10 LED circular bracket.
 */
static inline void draw_led_bracket(uint16_t x, uint16_t y)
{
	draw_hline(x + 2, y, 6, C_GRAY);
	draw_pixel(x + 1, y + 1, C_GRAY);
	draw_pixel(x + 8, y + 1, C_GRAY);
	draw_vline(x, y + 2, 6, C_GRAY);
	draw_vline(x + 9, y + 2, 6, C_GRAY);
	draw_pixel(x + 1, y + 8, C_GRAY);
	draw_pixel(x + 8, y + 8, C_GRAY);
	draw_hline(x + 2, y + 9, 6, C_GRAY);
}

/*
 *	Draw a LED inside a 10x10 circular bracket.
 */
static inline void draw_led(uint16_t x, uint16_t y, uint16_t col)
{
	int i;

	for (i = 1; i < 9; i++) {
		if (i == 1 || i == 8)
			draw_hline(x + 2, y + i, 6, col);
		else
			draw_hline(x + 1, y + i, 8, col);
	}
}

#endif /* !DRAW_INC */
