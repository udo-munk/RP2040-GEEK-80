/*
 * Z80SIM  -  a Z80-CPU simulator
 *
 * Copyright (C) 2024 by Udo Munk & Thomas Eberhardt
 *
 * This is the main program for a RP2040/RP2350-GEEK board,
 * substitutes z80core/simmain.c
 *
 * History:
 * 28-APR-2024 implemented first release of Z80 emulation
 * 09-MAY-2024 test 8080 emulation
 * 27-MAY-2024 add access to files on MicroSD
 * 28-MAY-2024 implemented boot from disk images with some OS
 * 31-MAY-2024 use USB UART
 * 09-JUN-2024 implemented boot ROM
 * 13-JUN-2024 ported to RP2040-GEEK
 * 15-JUN-2024 added access to RP2040-GEEK LCD display
 * 24-JUN-2024 added emulation of Cromemco Dazzler
 * 08-DEC-2024 ported to RP2350-GEEK
 */

/* Raspberry SDK and FatFS includes */
#include <stdio.h>
#include <string.h>
#if LIB_PICO_STDIO_USB || LIB_STDIO_MSC_USB
#include <tusb.h>
#endif
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/binary_info.h"
#include "hardware/adc.h"
#include "hardware/uart.h"
#include "hardware/watchdog.h"

#include "hw_config.h"
#include "my_rtc.h"

/* Project includes */
#include "sim.h"
#include "simdefs.h"
#include "simglb.h"
#include "simcfg.h"
#include "simmem.h"
#include "simcore.h"
#include "simport.h"
#include "simio.h"
#ifdef WANT_ICE
#include "simice.h"
#endif

#include "disks.h"
#include "draw.h"
#include "lcd.h"

#define BS  0x08 /* ASCII backspace */
#define DEL 0x7f /* ASCII delete */

/* CPU speed */
int speed = CPU_SPEED;

/* initial LCD status display */
int initial_lcd = LCD_STATUS_REGISTERS;

/*
 * callback for TinyUSB when terminal sends a break
 * stops CPU
 */
#if LIB_PICO_STDIO_USB || (LIB_STDIO_MSC_USB && !STDIO_MSC_USB_DISABLE_STDIO)
void tud_cdc_send_break_cb(uint8_t itf, uint16_t duration_ms)
{
	UNUSED(itf);
	UNUSED(duration_ms);

	cpu_error = USERINT;
	cpu_state = ST_STOPPED;
}
#endif

static const draw_banner_t banner[] = {
	{ "Z80pack " RELEASE, C_GREEN },
	{ MODEL " " USR_REL, C_RED },
	{ "by Udo Munk &", C_WHITE },
	{ "Thomas Eberhardt", C_WHITE },
	{ NULL, 0 }
};

static void lcd_draw_banner(int first)
{
	if (first)
		draw_banner(banner, &font28, C_BLUE);
}

#if LIB_PICO_STDIO_USB || (LIB_STDIO_MSC_USB && !STDIO_MSC_USB_DISABLE_STDIO)
static const draw_banner_t wait_term[] = {
	{ "Waiting for", C_RED },
	{ "terminal", C_RED },
	{ NULL, 0 }
};

static void lcd_draw_wait_term(int first)
{
	if (first)
		draw_banner(wait_term, &font28, C_WHITE);
}
#endif

int main(void)
{
	/* strings for picotool, so that it shows used pins */
	bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN,
				   PICO_DEFAULT_I2C_SCL_PIN,
				   GPIO_FUNC_I2C));
	char s[2];

	stdio_init_all();	/* initialize stdio */
#if LIB_STDIO_MSC_USB
	sd_init_driver();	/* initialize SD card driver */
	tusb_init();		/* initialize TinyUSB */
	stdio_msc_usb_init();	/* initialize MSC USB stdio */
#endif
	time_init();		/* initialize FatFS RTC */
	lcd_init();		/* initialize LCD */

	/*
	 * initialize hardware AD converter, enable onboard
	 * temperature sensor and select its channel
	 */
	adc_init();
	adc_set_temp_sensor_enabled(true);
	adc_select_input(4);

#if LIB_PICO_STDIO_UART
	uart_inst_t *my_uart = uart_default;
	/* destroy random input from UART after activation */
	if (uart_is_readable(my_uart))
		getchar();
#endif

	/* when using USB UART wait until it is connected */
	/* but also get out if there is input at default UART */
#if LIB_PICO_STDIO_USB || (LIB_STDIO_MSC_USB && !STDIO_MSC_USB_DISABLE_STDIO)
	lcd_custom_disp(lcd_draw_wait_term);
	while (!tud_cdc_connected()) {
#if LIB_PICO_STDIO_UART
		if (uart_is_readable(my_uart)) {
			getchar();
			break;
		}
#endif
		sleep_ms(100);
	}
#endif

	/* print banner */
	lcd_custom_disp(lcd_draw_banner);
	printf("\fZ80pack release %s, %s\n", RELEASE, COPYR);
	printf("%s release %s\n", USR_COM, USR_REL);
#if PICO_RP2350
#if PICO_RISCV
	puts("running on Hazard3 RISC-V cores");
#else
	puts("running on ARM Cortex-M33 cores");
#endif
#endif
	printf("%s\n\n", USR_CPR);

	init_cpu();		/* initialize CPU */
	init_disks();		/* initialize disk drives */
	init_memory();		/* initialize memory configuration */
	init_io();		/* initialize I/O devices */
	config();		/* configure the machine */

	f_flag = speed;		/* setup speed of the CPU */
	tmax = speed * 10000;	/* theoretically */

	PC = 0xff00;		/* power on jump into the boot ROM */
#ifdef SIMPLEPANEL
	fp_led_address = PC;
	fp_led_data = getmem(PC);
	cpu_bus = CPU_WO | CPU_M1 | CPU_MEMR;
#endif

	lcd_status_disp(initial_lcd); /* tell LCD task to display status */

	/* run the CPU with whatever is in memory */
#ifdef WANT_ICE
	ice_cmd_loop(0);
#else
	run_cpu();
#endif

	exit_disks();		/* stop disk drives */

#ifndef WANT_ICE
	putchar('\n');
	report_cpu_error();	/* check for CPU emulation errors and report */
	report_cpu_stats();	/* print some execution statistics */
#endif
	puts("\nPress any key to restart CPU");
	get_cmdline(s, 2);

	lcd_exit();		/* shutdown LCD */

	/* reset machine */
	watchdog_enable(1, 1);
	for (;;);
}

/*
 * Read an ICE or config command line of maximum length len - 1
 * from the terminal. For single character requests (len == 2),
 * returns immediately after input is received.
 */
int get_cmdline(char *buf, int len)
{
	int i = 0;
	char c;

	for (;;) {
		c = getchar();
		if ((c == BS) || (c == DEL)) {
			if (i >= 1) {
				putchar(BS);
				putchar(' ');
				putchar(BS);
				i--;
			}
		} else if (c != '\r') {
			if (i < len - 1) {
				buf[i++] = c;
				putchar(c);
				if (len == 2)
					break;
			}
		} else {
			break;
		}
	}
	buf[i] = '\0';
	putchar('\n');
	return 0;
}
