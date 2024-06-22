/*
 * Functions for using the RP2040-GEEK LCD from the emulation
 *
 * Copyright (C) 2024 by Udo Munk & Thomas Eberhardt
 */

#include "pico/multicore.h"
#include "lcd.h"
#include "sim.h"
#include "simglb.h"

/* memory image for drawing */
#if LCD_COLOR_DEPTH == 12
static UBYTE img[LCD_1IN14_V2_HEIGHT * ((LCD_1IN14_V2_WIDTH + 1) / 2) * 3];
#else
static UWORD img[LCD_1IN14_V2_HEIGHT * LCD_1IN14_V2_WIDTH];
#endif

static volatile int refresh_flag; /* task will refresh LCD display when > 0 */
static volatile int cpudisp_flag; /* task will draw CPU status when > 0 */
static int cpudisp_type;	/* currently shown CPU type */
static char info_line[25];	/* last line in display */

static void lcd_task(void);

void lcd_init(void)
{
	/* initialize LCD device */
	/* set orientation, x = 240 width and y = 135 height */
	LCD_1IN14_V2_Init(HORIZONTAL);

	/* use this image memory */
	Paint_NewImage((UBYTE *) &img[0], LCD_1IN14_V2.WIDTH,
			LCD_1IN14_V2.HEIGHT, 0, WHITE);

	/* with this depth */
	Paint_SetDepth(LCD_COLOR_DEPTH);

	/* 0,0 is upper left corner */
	Paint_SetRotate(ROTATE_0);

	cpudisp_flag = 0;
	cpudisp_type = cpu;
	refresh_flag = 1;
	snprintf(info_line, sizeof(info_line), "Z80pack RP2040-GEEK %s",
		 USR_REL);
	multicore_launch_core1(lcd_task);
}

void lcd_cpudisp_on(void)
{
	if (cpudisp_flag == 0)
		cpudisp_flag = 1;
}

void lcd_cpudisp_off(void)
{
	if (cpudisp_flag > 0) {
		/* tell LCD refresh task to stop displaying CPU registers */
		cpudisp_flag = -1;
		/* wait until it stopped */
		while (cpudisp_flag == -1)
			sleep_ms(20);

		Paint_Clear(BLACK);
	}
}

void lcd_finish(void)
{
	lcd_cpudisp_off();

	if (refresh_flag > 0) {
		/* tell LCD refresh task to stop refreshing the display */
		refresh_flag = -1;
		/* wait until it stopped */
		while (refresh_flag == -1)
			sleep_ms(20);
	}

	multicore_reset_core1();
}

static void lcd_refresh(void)
{
#if LCD_COLOR_DEPTH == 12
	LCD_1IN14_V2_Display12(&img[0]);
#else
	LCD_1IN14_V2_Display(&img[0]);
#endif
}

void lcd_brightness(int brightness)
{
	LCD_1IN14_V2_SetBacklight((uint8_t) brightness);
}

void lcd_info(void)
{
	Paint_Clear(BLACK);
	Paint_DrawString(32, 35, "Waiting for", &Font32, RED, BLACK);
	Paint_DrawString(32, 67, "  terminal ", &Font32, RED, BLACK);
}

void lcd_banner(void)
{
	Paint_Clear(BLACK);
	Paint_DrawString(32, 35, "# Z80pack #", &Font32, BLACK, WHITE);
	Paint_DrawString(32, 67, "RP2040-GEEK", &Font32, BLACK, WHITE);
}

/*
 *    012345678901234567890123
 *  0 A  12   BC 1234 DE 1234
 *  1 HL 1234 SP 1234 PC 1234
 *  2 IX 1234 IY 1234 AF`1234
 *  3 BC'1234 DE'1234 HL`1234
 *  4 F  SZHPNC  IF12 IR 1234
 *  5 z80pack RP2040-GEEK 1.2
 *
 *    0123456789012345
 *  0 A  12    BC 1234
 *  1 DE 1234  HL 1234
 *  2 SP 1234  PC 1234
 *  3 F  SZHPC    IF 1
 */

#if LCD_COLOR_DEPTH == 12
#define DKBLUE		0x0007	/* color used for background */
#define DKYELLOW	0x0880	/* color used for grid lines */
#else
#define DKBLUE		0x000f	/* color used for background */
#define DKYELLOW	0x8400	/* color used for grid lines */
#endif

#define OFFX20	5	/* x pixel offset of text coordinate grid for Font20 */
#define OFFY20	0	/* y pixel offset of text coordinate grid for Font20 */
#define SPC20	3	/* vertical spacing for Font20 */

#define OFFX28	8	/* x pixel offset of text coordinate grid for Font28 */
#define OFFY28	0	/* y pixel offset of text coordinate grid for Font28 */
#define SPC28	1	/* vertical spacing for Font28 */

/*
 * Draw character "c" at text coordinates "x, y" with color "col"
 */
#define P_C(x, y, c, col, font, offx, offy, spc)		\
	Paint_DrawChar((x) * font.Width + offx,			\
		       (y) * (font.Height + spc) + offy,	\
		       c, &font, col, DKBLUE)
#define P_C20(x, y, c, col) P_C(x, y, c, col, Font20, OFFX20, OFFY20, SPC20)
#define P_C28(x, y, c, col) P_C(x, y, c, col, Font28, OFFX28, OFFY28, SPC28)

/*
 * Draw string "s" at text coordinates "x, y" with color "col"
 */
#define P_S(x, y, s, col, font, offx, offy, spc)		\
	Paint_DrawString((x) * font.Width + offx,		\
			 (y) * (font.Height + spc) + offy,	\
			 s, &font, col, DKBLUE)
#define P_S20(x, y, s, col) P_S(x, y, s, col, Font20, OFFX20, OFFY20, SPC20)
#define P_S28(x, y, s, col) P_S(x, y, s, col, Font28, OFFX28, OFFY28, SPC28)

/*
 * Draw horizontal grid line in the middle of vertical spacing below
 * text row "y" with color "col" and vertical adjustment "adj"
 */
#define P_GH(y, adj, col, font, offy, spc)			\
	Paint_DrawLine(0,					\
		       ((y) + 1) * (font.Height + spc)		\
		       - (spc + 1) / 2 + 1 + offy + (adj),	\
		       Paint.Width - 1,				\
		       ((y) + 1) * (font.Height + spc)		\
		       - (spc + 1) / 2 + 1 + offy + (adj),	\
		       col, DOT_PIXEL_DFT, LINE_STYLE_SOLID)
#define P_GH20(y, adj, col) P_GH(y, adj, col, Font20, OFFY20, SPC20)
#define P_GH28(y, adj, col) P_GH(y, adj, col, Font28, OFFY28, SPC28)

/*
 * Draw vertical grid line in the middle of text column "x" with
 * color "col" from the top of the screen to the middle of vertical
 * spacing below text row "y" with vertical adjustment "adj"
 */
#define P_GV(x, y, adj, col, font, offx, offy, spc)			\
	Paint_DrawLine((x) * font.Width + font.Width / 2 + offx,	\
		       0,						\
		       (x) * font.Width + font.Width / 2 + offx,	\
		       ((y) + 1) * (font.Height + spc)			\
		       - (spc + 1) / 2 + 1 + offy + (adj),		\
		       col, DOT_PIXEL_DFT, LINE_STYLE_SOLID)
#define P_GV20(x, y, adj, col) \
	P_GV(x, y, adj, col, Font20, OFFX20, OFFY20, SPC20)
#define P_GV28(x, y, adj, col) \
	P_GV(x, y, adj, col, Font28, OFFX28, OFFY28, SPC28)

/*
 * Draw short vertical grid line in the middle of text column "x" with
 * color "col" from the middle of vertical spacing above text row "y0"
 * to the middle of vertical spacing below text row "y1" with vertical
 * adjustment "adj"
 */
#define P_GVS(x, y0, y1, adj, col, font, offx, offy, spc)		\
	Paint_DrawLine((x) * font.Width + font.Width / 2 + offx,	\
		       (y0) * (font.Height + spc)			\
		       - (spc + 1) / 2 + 1 + offy,			\
		       (x) * font.Width + font.Width / 2 + offx,	\
		       ((y1) + 1) * (font.Height + spc)			\
		       - (spc + 1) / 2 + 1 + offy + (adj),		\
		       col, DOT_PIXEL_DFT, LINE_STYLE_SOLID)
#define P_GVS20(x, y0, y1, adj, col) \
	P_GVS(x, y0, y1, adj, col, Font20, OFFX20, OFFY20, SPC20)
#define P_GVS28(x, y0, y1, adj, col) \
	P_GVS(x, y0, y1, adj, col, Font28, OFFX28, OFFY28, SPC28)

static const char *hex = "0123456789ABCDEF";

#define H3(x) hex[((x) >> 12) & 0xf]
#define H2(x) hex[((x) >> 8) & 0xf]
#define H1(x) hex[((x) >> 4) & 0xf]
#define H0(x) hex[(x) & 0xf]

static void lcd_cpubg(void)
{
	Paint_Clear(DKBLUE);
	if (cpudisp_type == Z80) {
		P_GV20(7, 3, 0, DKYELLOW);
		P_GVS20(10, 4, 4, 0, DKYELLOW);
		P_GV20(15, 4, 0, DKYELLOW);
		P_C20( 0, 0, 'A',   WHITE);
		P_S20( 8, 0, "BC",  WHITE);
		P_S20(16, 0, "DE",  WHITE);
		P_GH20(0, 0, DKYELLOW);
		P_S20( 0, 1, "HL",  WHITE);
		P_S20( 8, 1, "SP",  WHITE);
		P_S20(16, 1, "PC",  WHITE);
		P_GH20(1, 0, DKYELLOW);
		P_S20( 0, 2, "IX",  WHITE);
		P_S20( 8, 2, "IY",  WHITE);
		P_S20(16, 2, "AF'", WHITE);
		P_GH20(2, 0, DKYELLOW);
		P_S20( 0, 3, "BC'", WHITE);
		P_S20( 8, 3, "DE'", WHITE);
		P_S20(16, 3, "HL'", WHITE);
		P_GH20(3, 0, DKYELLOW);
		P_C20( 0, 4, 'F',   WHITE);
		P_S20(11, 4, "IF",  WHITE);
		P_S20(16, 4, "IR",  WHITE);
		P_GH20(4, 0, DKYELLOW);
	}
	else {
		/* adjustment keeps grid line outside "info_line" */
		P_GV28(8, 3, -2, DKYELLOW);
		P_C28( 0, 0, 'A',   WHITE);
		P_S28( 9, 0, "BC",  WHITE);
		P_GH28(0, 0, DKYELLOW);
		P_S28( 0, 1, "DE",  WHITE);
		P_S28( 9, 1, "HL",  WHITE);
		P_GH28(1, 0, DKYELLOW);
		P_S28( 0, 2, "SP",  WHITE);
		P_S28( 9, 2, "PC",  WHITE);
		P_GH28(2, 0, DKYELLOW);
		P_C28( 0, 3, 'F',   WHITE);
		P_S28(12, 3, "IF",  WHITE);
	}
	P_S20( 0, 5, info_line, BRRED);
}

static void lcd_cpudisp(void)
{
	BYTE r;

	if (cpudisp_type == Z80) {
		P_C20( 3, 0, H1(A),  GREEN); P_C20( 4, 0, H0(A),  GREEN);
		P_C20(11, 0, H1(B),  GREEN); P_C20(12, 0, H0(B),  GREEN);
		P_C20(13, 0, H1(C),  GREEN); P_C20(14, 0, H0(C),  GREEN);
		P_C20(19, 0, H1(D),  GREEN); P_C20(20, 0, H0(D),  GREEN);
		P_C20(21, 0, H1(E),  GREEN); P_C20(22, 0, H0(E),  GREEN);

		P_C20( 3, 1, H1(H),  GREEN); P_C20( 4, 1, H0(H),  GREEN);
		P_C20( 5, 1, H1(L),  GREEN); P_C20( 6, 1, H0(L),  GREEN);
		P_C20(11, 1, H3(SP), GREEN); P_C20(12, 1, H2(SP), GREEN);
		P_C20(13, 1, H1(SP), GREEN); P_C20(14, 1, H0(SP), GREEN);
		P_C20(19, 1, H3(PC), GREEN); P_C20(20, 1, H2(PC), GREEN);
		P_C20(21, 1, H1(PC), GREEN); P_C20(22, 1, H0(PC), GREEN);

		P_C20( 3, 2, H3(IX), GREEN); P_C20( 4, 2, H2(IX), GREEN);
		P_C20( 5, 2, H1(IX), GREEN); P_C20( 6, 2, H0(IX), GREEN);
		P_C20(11, 2, H3(IY), GREEN); P_C20(12, 2, H2(IY), GREEN);
		P_C20(13, 2, H1(IY), GREEN); P_C20(14, 2, H0(IY), GREEN);
		P_C20(19, 2, H1(A_), GREEN); P_C20(20, 2, H0(A_), GREEN);
		P_C20(21, 2, H1(F_), GREEN); P_C20(22, 2, H0(F_), GREEN);

		P_C20( 3, 3, H1(B_), GREEN); P_C20( 4, 3, H0(B_), GREEN);
		P_C20( 5, 3, H1(C_), GREEN); P_C20( 6, 3, H0(C_), GREEN);
		P_C20(11, 3, H1(D_), GREEN); P_C20(12, 3, H0(D_), GREEN);
		P_C20(13, 3, H1(E_), GREEN); P_C20(14, 3, H0(E_), GREEN);
		P_C20(19, 3, H1(H_), GREEN); P_C20(20, 3, H0(H_), GREEN);
		P_C20(21, 3, H1(L_), GREEN); P_C20(22, 3, H0(L_), GREEN);

		P_C20( 3, 4, 'S', F & S_FLAG ? GREEN : RED);
		P_C20( 4, 4, 'Z', F & Z_FLAG ? GREEN : RED);
		P_C20( 5, 4, 'H', F & H_FLAG ? GREEN : RED);
		P_C20( 6, 4, 'P', F & P_FLAG ? GREEN : RED);
		P_C20( 7, 4, 'N', F & N_FLAG ? GREEN : RED);
		P_C20( 8, 4, 'C', F & C_FLAG ? GREEN : RED);
		P_C20(13, 4, '1', IFF & 1 ? GREEN : RED);
		P_C20(14, 4, '2', IFF & 2 ? GREEN : RED);
		P_C20(19, 4, H1(I),  GREEN); P_C20(20, 4, H0(I),  GREEN);
		r = (R_ & 0x80) | (R & 0x7f);
		P_C20(21, 4, H1(r),  GREEN); P_C20(22, 4, H0(r),  GREEN);
	} else {
		P_C28( 3, 0, H1(A),  GREEN); P_C28( 4, 0, H0(A),  GREEN);
		P_C28(12, 0, H1(B),  GREEN); P_C28(13, 0, H0(B),  GREEN);
		P_C28(14, 0, H1(C),  GREEN); P_C28(15, 0, H0(C),  GREEN);

		P_C28( 3, 1, H1(D),  GREEN); P_C28( 4, 1, H0(D),  GREEN);
		P_C28( 5, 1, H1(E),  GREEN); P_C28( 6, 1, H0(E),  GREEN);
		P_C28(12, 1, H1(H),  GREEN); P_C28(13, 1, H0(H),  GREEN);
		P_C28(14, 1, H1(L),  GREEN); P_C28(15, 1, H0(L),  GREEN);

		P_C28( 3, 2, H3(SP), GREEN); P_C28( 4, 2, H2(SP), GREEN);
		P_C28( 5, 2, H1(SP), GREEN); P_C28( 6, 2, H0(SP), GREEN);
		P_C28(12, 2, H3(PC), GREEN); P_C28(13, 2, H2(PC), GREEN);
		P_C28(14, 2, H1(PC), GREEN); P_C28(15, 2, H0(PC), GREEN);

		P_C28( 3, 3, 'S', F & S_FLAG ? GREEN : RED);
		P_C28( 4, 3, 'Z', F & Z_FLAG ? GREEN : RED);
		P_C28( 5, 3, 'H', F & H_FLAG ? GREEN : RED);
		P_C28( 6, 3, 'P', F & P_FLAG ? GREEN : RED);
		P_C28( 7, 3, 'C', F & C_FLAG ? GREEN : RED);
		P_C28(15, 3, '1', IFF == 3 ? GREEN : RED);

		/*
		 * The adjustment moves the grid line from inside the
		 * "info_line" into the descenders space of the "F IF" line,
		 * therefore the need to draw it inside the refresh code
		 */
		P_GH28(3, -2, DKYELLOW);
	}
}

static void lcd_task(void)
{
	absolute_time_t t;
	int64_t d;

	while (1) {
		/* loops every LCD_REFRESH_US */

		t = get_absolute_time();

		if (cpudisp_flag) {
			if (cpudisp_flag < 0) {
				/* request to finish */
				cpudisp_flag = 0;
			} else {
				if (cpudisp_type != cpu) {
					cpudisp_type = cpu;
					cpudisp_flag = 1;
				}
				if (cpudisp_flag == 1) {
					/* first time here, draw background */
					lcd_cpubg();
					cpudisp_flag++;
				} else
					lcd_cpudisp();
			}
		}

		if (refresh_flag) {
			if (refresh_flag < 0) {
				/* request to finish */
				refresh_flag = 0;
			} else
				lcd_refresh();
		}

		d = absolute_time_diff_us(t, get_absolute_time());
		if (d < LCD_REFRESH_US)
			sleep_us(LCD_REFRESH_US - d);
		else
			puts("REFRESH!");
	}
}
