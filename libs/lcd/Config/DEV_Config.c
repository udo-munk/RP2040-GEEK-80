/*****************************************************************************
* | File	:   DEV_Config.c
* | Author	:
* | Function	:   Hardware underlying interface
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
# copies of theex Software, and to permit persons to  whom the Software is
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

#include "DEV_Config.h"

uint DEV_DMA_Channel;
bool DEV_DMA_Active;
void (*DEV_DMA_Done_Func)(void);
uint DEV_PWM_Slice_Num;

static void __not_in_flash_func(DEV_DMA_IRQ_Handler)(void)
{
	if (DEV_DMA_Active) { /* is there an active transfer from us? */
		if (DEV_DMA_IRQ == DMA_IRQ_0)
			dma_channel_acknowledge_irq0(DEV_DMA_Channel);
		else
			dma_channel_acknowledge_irq1(DEV_DMA_Channel);
		/* DMA transfer done doesn't mean that the SPI FIFO is empty */
		while (spi_is_busy(DEV_SPI_PORT))
			tight_loop_contents();
		DEV_DMA_Active = false;
		if (DEV_DMA_Done_Func != NULL) {
			(*DEV_DMA_Done_Func)();
			DEV_DMA_Done_Func = NULL;
		}
	}
}

void DEV_Module_Init(void)
{
	dma_channel_config c;

	/* SPI Config for LCD */
	/* 41.67 MHz on 125 MHz RP2040, 50 MHz on 150 MHz RP2350 */
	spi_init(DEV_SPI_PORT, clock_get_hz(clk_sys) / 3);
	gpio_set_function(WAVESHARE_GEEK_LCD_SCLK_PIN, GPIO_FUNC_SPI);
	gpio_set_function(WAVESHARE_GEEK_LCD_TX_PIN, GPIO_FUNC_SPI);

	/* GPIO Config for LCD */
	gpio_init(WAVESHARE_GEEK_LCD_RST_PIN);
	gpio_set_dir(WAVESHARE_GEEK_LCD_RST_PIN, GPIO_OUT);
	gpio_init(WAVESHARE_GEEK_LCD_DC_PIN);
	gpio_set_dir(WAVESHARE_GEEK_LCD_DC_PIN, GPIO_OUT);
	gpio_init(WAVESHARE_GEEK_LCD_CS_PIN);
	gpio_set_dir(WAVESHARE_GEEK_LCD_CS_PIN, GPIO_OUT);
	gpio_init(WAVESHARE_GEEK_LCD_BL_PIN);
	gpio_set_dir(WAVESHARE_GEEK_LCD_BL_PIN, GPIO_OUT);
	DEV_Digital_Write(WAVESHARE_GEEK_LCD_CS_PIN, 1);
	DEV_Digital_Write(WAVESHARE_GEEK_LCD_DC_PIN, 0);
	DEV_Digital_Write(WAVESHARE_GEEK_LCD_BL_PIN, 1);

	/* PWM Config for backlight */
	gpio_set_function(WAVESHARE_GEEK_LCD_BL_PIN, GPIO_FUNC_PWM);
	DEV_PWM_Slice_Num = pwm_gpio_to_slice_num(WAVESHARE_GEEK_LCD_BL_PIN);
	pwm_set_wrap(DEV_PWM_Slice_Num, 100);
	pwm_set_chan_level(DEV_PWM_Slice_Num, PWM_CHAN_B, 1);
	pwm_set_clkdiv(DEV_PWM_Slice_Num, 50);
	pwm_set_enabled(DEV_PWM_Slice_Num, true);

	/* DMA Config for framebuffer transfer */
	DEV_DMA_Active = false;
	DEV_DMA_Done_Func = NULL;
	DEV_DMA_Channel = dma_claim_unused_channel(true);
	c = dma_channel_get_default_config(DEV_DMA_Channel);
	channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
	channel_config_set_dreq(&c, spi_get_dreq(DEV_SPI_PORT, true));
	dma_channel_set_config(DEV_DMA_Channel, &c, false);
	dma_channel_set_write_addr(DEV_DMA_Channel,
				   &spi_get_hw(DEV_SPI_PORT)->dr, false);
	if (DEV_DMA_IRQ == DMA_IRQ_0)
		dma_channel_set_irq0_enabled(DEV_DMA_Channel, true);
	else
		dma_channel_set_irq1_enabled(DEV_DMA_Channel, true);
	irq_add_shared_handler(DEV_DMA_IRQ, DEV_DMA_IRQ_Handler,
			       PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
	irq_set_enabled(DEV_DMA_IRQ, true);
}

void DEV_Module_Exit(void)
{
	DEV_Wait_DMA_Done();

	irq_set_enabled(DEV_DMA_IRQ, false);
	irq_remove_handler(DEV_DMA_IRQ, DEV_DMA_IRQ_Handler);
	dma_channel_cleanup(DEV_DMA_Channel); /* also disables interrupt */
	dma_channel_unclaim(DEV_DMA_Channel);

	pwm_set_enabled(DEV_PWM_Slice_Num, false);
	gpio_deinit(WAVESHARE_GEEK_LCD_BL_PIN);

	gpio_deinit(WAVESHARE_GEEK_LCD_DC_PIN);
	gpio_deinit(WAVESHARE_GEEK_LCD_CS_PIN);
	gpio_deinit(WAVESHARE_GEEK_LCD_RST_PIN);

	spi_deinit(DEV_SPI_PORT);
	gpio_deinit(WAVESHARE_GEEK_LCD_SCLK_PIN);
	gpio_deinit(WAVESHARE_GEEK_LCD_TX_PIN);
}
