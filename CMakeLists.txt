cmake_minimum_required(VERSION 3.13)

# Set default build type to Release
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
set(CMAKE_BUILD_TYPE Release)
endif()

set(PICO_BOARD_HEADER_DIRS ${CMAKE_SOURCE_DIR})

# Set platform, board, and compiler
set(PICO_PLATFORM rp2040)
set(PICO_BOARD waveshare_rp2040_geek)
set(PICO_COMPILER pico_arm_gcc)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(picosim C CXX ASM)

pico_sdk_init()

set(Z80PACK ${CMAKE_SOURCE_DIR}/../z80pack)

add_executable(${PROJECT_NAME}
	picosim.c
	dazzler.c
	disks.c
	lcd.c
	simcfg.c
	simio.c
	simmem.c
	${Z80PACK}/iodevices/rtc80.c
	${Z80PACK}/iodevices/sd-fdc.c
	${Z80PACK}/z80core/sim8080.c
	${Z80PACK}/z80core/simcore.c
	${Z80PACK}/z80core/simdis.c
	${Z80PACK}/z80core/simglb.c
	${Z80PACK}/z80core/simice.c
	${Z80PACK}/z80core/simutil.c
	${Z80PACK}/z80core/simz80-cb.c
	${Z80PACK}/z80core/simz80-dd.c
	${Z80PACK}/z80core/simz80-ddcb.c
	${Z80PACK}/z80core/simz80-ed.c
	${Z80PACK}/z80core/simz80-fd.c
	${Z80PACK}/z80core/simz80-fdcb.c
	${Z80PACK}/z80core/simz80.c
)

target_include_directories(${PROJECT_NAME} PUBLIC
	${CMAKE_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/lcd/Config
	${CMAKE_SOURCE_DIR}/lcd/Fonts
	${CMAKE_SOURCE_DIR}/lcd/GUI
	${CMAKE_SOURCE_DIR}/lcd/LCD
	${Z80PACK}/iodevices
	${Z80PACK}/z80core
)

add_subdirectory(no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/src FatFs)
add_subdirectory(lcd/Config)
add_subdirectory(lcd/LCD)
add_subdirectory(lcd/Fonts)
add_subdirectory(lcd/GUI)
add_subdirectory(stdio_msc_usb)
add_subdirectory(ds3231)

target_compile_definitions(${PROJECT_NAME} PRIVATE
	PICO_STACK_SIZE=4096
	PICO_CORE1_STACK_SIZE=4096
	PICO_HEAP_SIZE=8192
	# LCD frame buffer color depth (12 or 16 bits)
	LCD_COLOR_DEPTH=12
	# LCD refresh rate in Hz (30 works well with 12-bit frame buffer)
	LCD_REFRESH=30
	# the official VID/PID assigned to RP2040-GEEK-80 from Raspberry Pi
	# see: https://github.com/raspberrypi/usb-pid
	USBD_VID=0x2E8A
	USBD_PID=0x1095
)

# compiler diagnostic options
if (PICO_C_COMPILER_IS_GNU)
	target_compile_options(${PROJECT_NAME} PUBLIC -fdiagnostics-color=always)
elseif (PICO_C_COMPILER_IS_CLANG)
	target_compile_options(${PROJECT_NAME} PUBLIC -fcolor-diagnostics)
endif ()
target_compile_options(${PROJECT_NAME} PUBLIC -Wall -Wextra)
if (PICO_C_COMPILER_IS_CLANG)
	target_compile_options(${PROJECT_NAME} PUBLIC -Weverything)
endif ()
# -Wno-unused-parameter because no-OS-FatFS-SD-SDIO-SPI-RPi-Pico isn't clean
target_compile_options(${PROJECT_NAME} PUBLIC -Wno-unused-parameter)

target_link_libraries(${PROJECT_NAME}
	hardware_adc
	hardware_spi
	hardware_i2c
	pico_multicore
	pico-ds3231
	pico_stdlib
	stdio_msc_usb
	tinyusb_device
	no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
	Config
	Fonts
	GUI
	LCD
)

pico_add_extra_outputs(${PROJECT_NAME})

target_link_options(${PROJECT_NAME} PRIVATE -Xlinker --print-memory-usage)

pico_set_program_name(${PROJECT_NAME} "z80pack picosim")
pico_set_program_description(${PROJECT_NAME} "z80pack on Waveshare RP2040-GEEK")
pico_set_program_version(${PROJECT_NAME} "1.3")
pico_set_program_url(${PROJECT_NAME} "https://github.com/udo-munk/RP2040-GEEK-80")

# disable UART in/out, enable USB in/out
pico_enable_stdio_uart(${PROJECT_NAME} 1)
#pico_enable_stdio_usb(${PROJECT_NAME} 1)
