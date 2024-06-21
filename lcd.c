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
static int curr_cpu;		/* currently shown CPU type */
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
	refresh_flag = 1;
	curr_cpu = cpu;
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
 *  4 F  S Z H P N C  IR 1234
 *  5 z80pack RP2040-GEEK 1.2
 */

#define FONT	Font20
#define OFF_X	8
#define OFF_Y	0
#define SPC_Y	3

#define P_C(x, y, c, col) \
	Paint_DrawChar((x) * FONT.Width + OFF_X,		\
		       (y) * (FONT.Height + SPC_Y) + OFF_Y,	\
		       c, &FONT, col, BLACK)
#define P_S(x, y, s, col) \
	Paint_DrawString((x) * FONT.Width + OFF_X,		\
			 (y) * (FONT.Height + SPC_Y) + OFF_Y,	\
			 s, &FONT, col, BLACK)

static const char *hex = "0123456789ABCDEF";

#define H3(x) hex[((x) >> 12) & 0xf]
#define H2(x) hex[((x) >> 8) & 0xf]
#define H1(x) hex[((x) >> 4) & 0xf]
#define H0(x) hex[(x) & 0xf]

static void lcd_cpubg(void)
{
	Paint_Clear(BLACK);
	P_C( 0, 0, 'A',  WHITE);
	P_S( 8, 0, "BC", WHITE);
	P_S(16, 0, "DE", WHITE);
	P_S( 0, 1, "HL", WHITE);
	P_S( 8, 1, "SP", WHITE);
	P_S(16, 1, "PC", WHITE);
	if (curr_cpu == Z80) {
		P_S( 0, 2, "IX",  WHITE);
		P_S( 8, 2, "IY",  WHITE);
		P_S(16, 2, "AF'", WHITE);
		P_S( 0, 3, "BC'", WHITE);
		P_S( 8, 3, "DE'", WHITE);
		P_S(16, 3, "HL'", WHITE);
		P_C( 0, 4, 'F',   WHITE);
		P_S(16, 4, "IR",  WHITE);
	}
	else {
		P_C( 0, 3, 'F',   WHITE);
	}
	P_S( 0, 5, info_line, BRRED);
}

static void lcd_cpudisp(void)
{
	BYTE r;

	P_C( 3, 0, H1(A),  GREEN); P_C( 4, 0, H0(A),  GREEN);
	P_C(11, 0, H1(B),  GREEN); P_C(12, 0, H0(B),  GREEN);
	P_C(13, 0, H1(C),  GREEN); P_C(14, 0, H0(C),  GREEN);
	P_C(19, 0, H1(D),  GREEN); P_C(20, 0, H0(D),  GREEN);
	P_C(21, 0, H1(E),  GREEN); P_C(22, 0, H0(E),  GREEN);

	P_C( 3, 1, H1(H),  GREEN); P_C( 4, 1, H0(H),  GREEN);
	P_C( 5, 1, H1(L),  GREEN); P_C( 6, 1, H0(L),  GREEN);
	P_C(11, 1, H3(SP), GREEN); P_C(12, 1, H2(SP), GREEN);
	P_C(13, 1, H1(SP), GREEN); P_C(14, 1, H0(SP), GREEN);
	P_C(19, 1, H3(PC), GREEN); P_C(20, 1, H2(PC), GREEN);
	P_C(21, 1, H1(PC), GREEN); P_C(22, 1, H0(PC), GREEN);

	if (curr_cpu == Z80) {
		P_C( 3, 2, H3(IX), GREEN); P_C( 4, 2, H2(IX), GREEN);
		P_C( 5, 2, H1(IX), GREEN); P_C( 6, 2, H0(IX), GREEN);
		P_C(11, 2, H3(IY), GREEN); P_C(12, 2, H2(IY), GREEN);
		P_C(13, 2, H1(IY), GREEN); P_C(14, 2, H0(IY), GREEN);
		P_C(19, 2, H1(A_), GREEN); P_C(20, 2, H1(A_), GREEN);
		P_C(21, 2, H1(F_), GREEN); P_C(22, 2, H0(F_), GREEN);

		P_C( 3, 3, H1(B_), GREEN); P_C( 4, 3, H1(B_), GREEN);
		P_C( 5, 3, H1(C_), GREEN); P_C( 6, 3, H0(C_), GREEN);
		P_C(11, 3, H1(D_), GREEN); P_C(12, 3, H1(D_), GREEN);
		P_C(13, 3, H1(E_), GREEN); P_C(14, 3, H0(E_), GREEN);
		P_C(19, 3, H1(H_), GREEN); P_C(20, 3, H1(H_), GREEN);
		P_C(21, 3, H1(L_), GREEN); P_C(22, 3, H0(L_), GREEN);

		P_C( 3, 4, 'S', F & S_FLAG ? GREEN : RED);
		P_C( 5, 4, 'Z', F & Z_FLAG ? GREEN : RED);
		P_C( 7, 4, 'H', F & H_FLAG ? GREEN : RED);
		P_C( 9, 4, 'P', F & P_FLAG ? GREEN : RED);
		P_C(11, 4, 'N', F & N_FLAG ? GREEN : RED);
		P_C(13, 4, 'C', F & C_FLAG ? GREEN : RED);
		P_C(19, 4, H1(I), GREEN); P_C(20, 4, H0(I), GREEN);
		r = (R_ & 0x80) | (R & 0x7f);
		P_C(21, 4, H1(r), GREEN); P_C(22, 4, H0(r), GREEN);
	} else {
		P_C( 3, 3, 'S', F & S_FLAG ? GREEN : RED);
		P_C( 5, 3, 'Z', F & Z_FLAG ? GREEN : RED);
		P_C( 7, 3, 'H', F & H_FLAG ? GREEN : RED);
		P_C( 9, 3, 'P', F & P_FLAG ? GREEN : RED);
		P_C(11, 3, 'C', F & C_FLAG ? GREEN : RED);
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
				if (curr_cpu != cpu) {
					curr_cpu = cpu;
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
