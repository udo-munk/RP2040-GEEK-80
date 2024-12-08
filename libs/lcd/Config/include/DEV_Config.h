/*****************************************************************************
* | File	:   DEV_Config.h
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

#ifndef _DEV_CONFIG_H_
#define _DEV_CONFIG_H_

#include <stdint.h>
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "hardware/pwm.h"
#include "pico/time.h"

#define DEV_SPI_PORT (__CONCAT(spi,WAVESHARE_GEEK_LCD_SPI))

extern uint DEV_pwm_slice_num;

/*---------------------------------------------------------------------------*/

/*
 *	GPIO read and write
 */
static inline void DEV_Digital_Write(uint16_t Pin, uint8_t Value)
{
	gpio_put(Pin, Value);
}

static inline uint8_t DEV_Digital_Read(uint16_t Pin)
{
	return gpio_get(Pin);
}

/*
 *	SPI write
 */
static inline void DEV_SPI_WriteByte(uint8_t Value)
{
	spi_write_blocking(DEV_SPI_PORT, &Value, 1);
}

static inline void DEV_SPI_Write_nByte(uint8_t *pData, uint32_t Len)
{
	spi_write_blocking(DEV_SPI_PORT, pData, Len);
}

/*
 *	Delay x ms
 */
static inline void DEV_Delay_ms(uint32_t xms)
{
	sleep_ms(xms);
}

/*
 *	Set PWM
 */
static inline void DEV_Set_PWM(uint8_t Value)
{
	if (Value <= 100)
		pwm_set_chan_level(DEV_pwm_slice_num, PWM_CHAN_B, Value);
}

extern void DEV_Module_Init(void);
extern void DEV_Module_Exit(void);

#endif
