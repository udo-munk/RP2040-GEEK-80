/*
 * Functions for communicating with the RP2040/RP2350-GEEK LCD
 * (loosely based on Waveshare example code)
 *
 * Copyright (C) 2024 by Thomas Eberhardt
 */

#include <stdint.h>

#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/spi.h"
#include "hardware/sync.h"
#include "pico/time.h"

#include "lcd_dev.h"
#include "draw.h"

#define LCD_SPI		(__CONCAT(spi,WAVESHARE_GEEK_LCD_SPI))
#define LCD_DMA_IRQ	(DMA_IRQ_1)

static bool lcd_rotated;
static uint lcd_dma_channel;
static bool lcd_dma_active;
static uint lcd_pwm_slice_num;

static void lcd_dma_irq_handler(void);
static void lcd_dma_wait(void);

/*
 *	Send command to LCD controller
 */
static void __not_in_flash_func(lcd_dev_send_cmd)(uint8_t reg)
{
	gpio_put(WAVESHARE_GEEK_LCD_DC_PIN, 0);
	gpio_put(WAVESHARE_GEEK_LCD_CS_PIN, 0);
	spi_write_blocking(LCD_SPI, &reg, 1);
	gpio_put(WAVESHARE_GEEK_LCD_CS_PIN, 1);
}

/*
 *	Send 8-bit value to LCD controller
 */
static void __not_in_flash_func(lcd_dev_send_byte)(uint8_t data)
{
	gpio_put(WAVESHARE_GEEK_LCD_DC_PIN, 1);
	gpio_put(WAVESHARE_GEEK_LCD_CS_PIN, 0);
	spi_write_blocking(LCD_SPI, &data, 1);
	gpio_put(WAVESHARE_GEEK_LCD_CS_PIN, 1);
}

/*
 *	Send 16-bit value to LCD controller
 */
static void __not_in_flash_func(lcd_dev_send_word)(uint16_t data)
{
	uint8_t h = (data >> 8) & 0xff, l = data & 0xff;

	gpio_put(WAVESHARE_GEEK_LCD_DC_PIN, 1);
	gpio_put(WAVESHARE_GEEK_LCD_CS_PIN, 0);
	spi_write_blocking(LCD_SPI, &h, 1);
	spi_write_blocking(LCD_SPI, &l, 1);
	gpio_put(WAVESHARE_GEEK_LCD_CS_PIN, 1);
}

/*
 *	LCD controller register initialization table
 */
static const uint8_t lcd_init_tab[] = {
	/* cmd, nargs, args... */
	/* nargs with bit 7 set delays 100 ms after command is sent */
	0x36, 1, 0x70,				/* Memory Data Access Control */
	0xb2, 5, 0x0c, 0x0c, 0x00, 0x33, 0x33,	/* Porch Setting */
	0xb7, 1, 0x35,				/* Gate Control */
	0xbb, 1, 0x19,				/* VCOM Setting */
	0xc0, 1, 0x2c,				/* LCM Control */
	0xc2, 1, 0x01,				/* VDV & VRH Command Enable */
	0xc3, 1, 0x12,				/* VRH Set */
	0xc4, 1, 0x20,				/* VDV Set */
	0xc5, 1, 0x20,				/* VCOM Offset Set */
	0xc6, 1, 0x0f,				/* FRC in Normal Mode */
	0xd0, 2, 0xa4, 0xa1,			/* Power Control 1 */
	0xe0, 14, 0xd0, 0x04, 0x0d, 0x11, 0x13,	/* Pos Voltage Gamma Control */
		0x2b, 0x3f, 0x54, 0x4c, 0x18,
		0x0d, 0x0b, 0x1f, 0x23,
	0xe1, 14, 0xd0, 0x04, 0x0c, 0x11, 0x13,	/* Neg Voltage Gamma Control */
		0x2c, 0x3f, 0x44, 0x51, 0x2f,
		0x1f, 0x1f, 0x20, 0x23,
	0x21, 0,				/* Display Inversion On */
	0x11, 0 | 0x80,				/* Sleep Out */
	0x29, 0 | 0x80,				/* Display On */
	0xff
};

/*
 *	Initialize LCD controller
 */
void lcd_dev_init(void)
{
	dma_channel_config c;
	const uint8_t *p;
	int i, n;

	/*
	 * ST7789VW datasheet says 16 ns minimum serial write clock cycle,
	 * so 50 MHz (20 ns) should be OK.
	 */

	/* SPI Config for LCD controller */
	/* 41.67 MHz on 125 MHz RP2040, 50 MHz on 150 MHz RP2350 */
	spi_init(LCD_SPI, clock_get_hz(clk_sys) / 3);
	gpio_set_function(WAVESHARE_GEEK_LCD_SCLK_PIN, GPIO_FUNC_SPI);
	gpio_set_function(WAVESHARE_GEEK_LCD_TX_PIN, GPIO_FUNC_SPI);

	/* GPIO Config for LCD controller */
	gpio_init(WAVESHARE_GEEK_LCD_RST_PIN);
	gpio_set_dir(WAVESHARE_GEEK_LCD_RST_PIN, GPIO_OUT);
	gpio_init(WAVESHARE_GEEK_LCD_DC_PIN);
	gpio_set_dir(WAVESHARE_GEEK_LCD_DC_PIN, GPIO_OUT);
	gpio_init(WAVESHARE_GEEK_LCD_CS_PIN);
	gpio_set_dir(WAVESHARE_GEEK_LCD_CS_PIN, GPIO_OUT);
	gpio_init(WAVESHARE_GEEK_LCD_BL_PIN);
	gpio_set_dir(WAVESHARE_GEEK_LCD_BL_PIN, GPIO_OUT);
	gpio_put(WAVESHARE_GEEK_LCD_CS_PIN, 1);
	gpio_put(WAVESHARE_GEEK_LCD_DC_PIN, 0);
	gpio_put(WAVESHARE_GEEK_LCD_BL_PIN, 1);

	/* PWM Config for LCD backlight */
	gpio_set_function(WAVESHARE_GEEK_LCD_BL_PIN, GPIO_FUNC_PWM);
	lcd_pwm_slice_num = pwm_gpio_to_slice_num(WAVESHARE_GEEK_LCD_BL_PIN);
	pwm_set_wrap(lcd_pwm_slice_num, 100);
	pwm_set_chan_level(lcd_pwm_slice_num, PWM_CHAN_B, 1);
	pwm_set_clkdiv(lcd_pwm_slice_num, 50);
	pwm_set_enabled(lcd_pwm_slice_num, true);

	/* DMA Config for pixmap transfer */
	lcd_dma_active = false;
	lcd_dma_channel = dma_claim_unused_channel(true);
	c = dma_channel_get_default_config(lcd_dma_channel);
	channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
	channel_config_set_dreq(&c, spi_get_dreq(LCD_SPI, true));
	dma_channel_set_config(lcd_dma_channel, &c, false);
	dma_channel_set_write_addr(lcd_dma_channel,
				   &spi_get_hw(LCD_SPI)->dr, false);
	if (LCD_DMA_IRQ == DMA_IRQ_0)
		dma_channel_set_irq0_enabled(lcd_dma_channel, true);
	else
		dma_channel_set_irq1_enabled(lcd_dma_channel, true);
	irq_add_shared_handler(LCD_DMA_IRQ, lcd_dma_irq_handler,
			       PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
	irq_set_enabled(LCD_DMA_IRQ, true);

	/* reset the LCD controller */
	gpio_put(WAVESHARE_GEEK_LCD_RST_PIN, 1);
	sleep_ms(100);
	gpio_put(WAVESHARE_GEEK_LCD_RST_PIN, 0);
	sleep_ms(100);
	gpio_put(WAVESHARE_GEEK_LCD_RST_PIN, 1);
	sleep_ms(100);

	/* set LCD backlight intensity */
	lcd_dev_backlight(90);

	/* initialize LCD controller registers */
	for (p = lcd_init_tab; *p != 0xff;) {
		lcd_dev_send_cmd(*p++);
		n = *p++;
		for (i = n & 0x7f; i; i--)
			lcd_dev_send_byte(*p++);
		if (n & 0x80)
			sleep_ms(100);
	}

	lcd_rotated = false;
}

/*
 *	Deinitialize LCD controller
 */
void lcd_dev_exit(void)
{
	lcd_dma_wait();

	/* turn off the LCD backlight */
	lcd_dev_backlight(0);

	lcd_dev_send_cmd(0x28);			/* Display Off */
	sleep_ms(100);
	lcd_dev_send_cmd(0x10);			/* Sleep In */
	sleep_ms(100);

	irq_set_enabled(LCD_DMA_IRQ, false);
	irq_remove_handler(LCD_DMA_IRQ, lcd_dma_irq_handler);
	dma_channel_cleanup(lcd_dma_channel); /* also disables interrupt */
	dma_channel_unclaim(lcd_dma_channel);

	pwm_set_enabled(lcd_pwm_slice_num, false);
	gpio_deinit(WAVESHARE_GEEK_LCD_BL_PIN);

	gpio_deinit(WAVESHARE_GEEK_LCD_DC_PIN);
	gpio_deinit(WAVESHARE_GEEK_LCD_CS_PIN);
	gpio_deinit(WAVESHARE_GEEK_LCD_RST_PIN);

	spi_deinit(LCD_SPI);
	gpio_deinit(WAVESHARE_GEEK_LCD_SCLK_PIN);
	gpio_deinit(WAVESHARE_GEEK_LCD_TX_PIN);
}

/*
 *	Set LCD backlight intensity (0 - 100)
 */
void lcd_dev_backlight(uint8_t value)
{
	if (value <= 100)
		pwm_set_chan_level(lcd_pwm_slice_num, PWM_CHAN_B, value);
}

/*
 *	Set LCD rotation mode
 */
void lcd_dev_rotated(uint8_t rotated)
{
	lcd_dma_wait();

	lcd_dev_send_cmd(0x36);		/* Memory Data Access Control */
	if (rotated) {
		lcd_dev_send_byte(0xb0); /* MY=1, MX=0, MV=1, ML=1 */
		lcd_rotated = true;
	} else {
		lcd_dev_send_byte(0x70); /* MY=0, MX=1, MV=1, ML=1 */
		lcd_rotated = false;
	}
}

/*
 *	DMA transfer interrupt handler
 */
static void __not_in_flash_func(lcd_dma_irq_handler)(void)
{
	if (lcd_dma_active) { /* is there an active transfer from us? */
		if (LCD_DMA_IRQ == DMA_IRQ_0)
			dma_channel_acknowledge_irq0(lcd_dma_channel);
		else
			dma_channel_acknowledge_irq1(lcd_dma_channel);
		/* DMA transfer done doesn't mean that the SPI FIFO is empty */
		while (spi_is_busy(LCD_SPI))
			tight_loop_contents();
		gpio_put(WAVESHARE_GEEK_LCD_CS_PIN, 1);
		lcd_dma_active = false;
	}
}

/*
 *	Wait for DMA transfer to finish
 */
static void __not_in_flash_func(lcd_dma_wait)(void)
{
	while (lcd_dma_active)
		__wfi();
}

/*
 *	Send a pixmap to the LCD controller using DMA
 */
void __not_in_flash_func(lcd_dev_display)(draw_pixmap_t *pixmap)
{
	uint8_t x = 40, y = lcd_rotated ? 52 : 53;

	lcd_dma_wait();

	lcd_dev_send_cmd(0x3a);		/* Interface Pixel Format */
#if COLOR_DEPTH == 12
	lcd_dev_send_byte(0x03);	/* 12-bit */
#else
	lcd_dev_send_byte(0x05);	/* 16-bit */
#endif
	lcd_dev_send_cmd(0x2a);		/* Column Address Set */
	lcd_dev_send_word(x);
	lcd_dev_send_word(x + pixmap->width - 1);
	lcd_dev_send_cmd(0x2b);		/* Row Address Set */
	lcd_dev_send_word(y);
	lcd_dev_send_word(y + pixmap->height - 1);
	lcd_dev_send_cmd(0x2c);		/* Memory Write */
	gpio_put(WAVESHARE_GEEK_LCD_DC_PIN, 1);
	gpio_put(WAVESHARE_GEEK_LCD_CS_PIN, 0);
	lcd_dma_active = true;
	dma_channel_transfer_from_buffer_now(lcd_dma_channel, pixmap->bits,
					     (uint32_t) pixmap->stride *
					     pixmap->height);
}
