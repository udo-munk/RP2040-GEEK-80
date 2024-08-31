/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

// pico_cmake_set PICO_PLATFORM=rp2040

#ifndef _BOARDS_WAVESHARE_RP2040_GEEK_H
#define _BOARDS_WAVESHARE_RP2040_GEEK_H

// For board detection
#define WAVESHARE_RP2040_GEEK

// --- UART ---
#ifndef PICO_DEFAULT_UART
#define PICO_DEFAULT_UART 1
#endif
#ifndef PICO_DEFAULT_UART_TX_PIN
#define PICO_DEFAULT_UART_TX_PIN 4
#endif
#ifndef PICO_DEFAULT_UART_RX_PIN
#define PICO_DEFAULT_UART_RX_PIN 5
#endif

// Has debug probe pins on GPIO2 and GPIO3

// no PICO_DEFAULT_LED_PIN
// no PICO_DEFAULT_WS2812_PIN

// --- I2C ---
#ifndef PICO_DEFAULT_I2C
#define PICO_DEFAULT_I2C i2c0
#endif
#ifndef PICO_DEFAULT_I2C_SDA_PIN
#define PICO_DEFAULT_I2C_SDA_PIN 28
#endif
#ifndef PICO_DEFAULT_I2C_SCL_PIN
#define PICO_DEFAULT_I2C_SCL_PIN 29
#endif

// --- SD CARD ---
#ifndef PICO_SD_CLK_PIN
#define PICO_SD_CLK_PIN 18
#endif
#ifndef PICO_SD_CMD_PIN
#define PICO_SD_CMD_PIN 19
#endif
#ifndef PICO_SD_DAT0_PIN
#define PICO_SD_DAT0_PIN 20
#endif
// 1 or 4
#ifndef PICO_SD_DAT_PIN_COUNT
#define PICO_SD_DAT_PIN_COUNT 4
#endif
// 1 or -1
#ifndef PICO_SD_DAT_PIN_INCREMENT
#define PICO_SD_DAT_PIN_INCREMENT 1
#endif

// --- LCD ---
#ifndef WAVESHARE_RP2040_LCD_SPI
#define WAVESHARE_RP2040_LCD_SPI 1
#endif
#ifndef WAVESHARE_RP2040_LCD_DC_PIN
#define WAVESHARE_RP2040_LCD_DC_PIN 8
#endif
#ifndef WAVESHARE_RP2040_LCD_CS_PIN
#define WAVESHARE_RP2040_LCD_CS_PIN 9
#endif
#ifndef WAVESHARE_RP2040_LCD_SCLK_PIN
#define WAVESHARE_RP2040_LCD_SCLK_PIN 10
#endif
#ifndef WAVESHARE_RP2040_LCD_TX_PIN
#define WAVESHARE_RP2040_LCD_TX_PIN 11
#endif
#ifndef WAVESHARE_RP2040_LCD_RST_PIN
#define WAVESHARE_RP2040_LCD_RST_PIN 12
#endif
#ifndef WAVESHARE_RP2040_LCD_BL_PIN
#define WAVESHARE_RP2040_LCD_BL_PIN 25
#endif

// --- FLASH ---

#define PICO_BOOT_STAGE2_CHOOSE_W25Q080 1

#ifndef PICO_FLASH_SPI_CLKDIV
#define PICO_FLASH_SPI_CLKDIV 2
#endif

// pico_cmake_set_default PICO_FLASH_SIZE_BYTES = (4 * 1024 * 1024)
#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (4 * 1024 * 1024)
#endif

// All boards have B1 RP2040
#ifndef PICO_RP2040_B0_SUPPORTED
#define PICO_RP2040_B0_SUPPORTED  0
#endif

#endif
