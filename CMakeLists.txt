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

include_directories(
	.
	../z80pack/z80core
	../z80pack/iodevices
	./lcd/Config
	./lcd/Fonts
	./lcd/GUI
	./lcd/LCD
)

add_executable(picosim
	picosim.c
	../z80pack/z80core/simcore.c
	../z80pack/z80core/simglb.c
	../z80pack/z80core/simdis.c
	../z80pack/z80core/simice.c
	../z80pack/z80core/sim8080.c
	../z80pack/z80core/simz80.c
	../z80pack/z80core/simz80-cb.c
	../z80pack/z80core/simz80-dd.c
	../z80pack/z80core/simz80-ddcb.c
	../z80pack/z80core/simz80-ed.c
	../z80pack/z80core/simz80-fd.c
	../z80pack/z80core/simz80-fdcb.c
	../z80pack/iodevices/sd-fdc.c
	config.c
	dazzler.c
	memsim.c
	iosim.c
	lcd.c
)

add_subdirectory(no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/src FatFs)
add_subdirectory(lcd/Config)
add_subdirectory(lcd/LCD)
add_subdirectory(lcd/Fonts)
add_subdirectory(lcd/GUI)

target_compile_definitions(picosim PRIVATE
	PICO_STACK_SIZE=4096
	PICO_CORE1_STACK_SIZE=4096
	PICO_HEAP_SIZE=8192
	# LCD frame buffer color depth (12 or 16 bits)
	LCD_COLOR_DEPTH=12
	# LCD refresh rate in Hz (30 works well with 12-bit frame buffer)
	LCD_REFRESH=30
)

target_compile_definitions(picosim INTERFACE
	PICO_DEFAULT_UART=0
)

# compiler diagnostic options
if (PICO_C_COMPILER_IS_GNU)
	target_compile_options(picosim PUBLIC -fdiagnostics-color=always)
elseif (PICO_C_COMPILER_IS_CLANG)
	target_compile_options(picosim PUBLIC -fcolor-diagnostics)
endif ()
target_compile_options(picosim PUBLIC -Wall -Wextra)
if (PICO_C_COMPILER_IS_CLANG)
	target_compile_options(picosim PUBLIC -Weverything)
endif ()
# -Wno-unused-parameter because no-OS-FatFS-SD-SDIO-SPI-RPi-Pico isn't clean
target_compile_options(picosim PUBLIC -Wno-unused-parameter)

target_link_libraries(picosim
	hardware_adc
	hardware_spi
	pico_multicore
	pico_stdlib
	no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
	Config
	LCD
	Fonts
	GUI
)

pico_add_extra_outputs(picosim)

target_link_options(picosim PRIVATE -Xlinker --print-memory-usage)

pico_set_program_name(picosim "z80pack picosim")
pico_set_program_description(picosim "z80pack on Waveshare RP2040-GEEK")
pico_set_program_version(picosim "1.2")
pico_set_program_url(picosim "https://github.com/udo-munk/RP2040-GEEK-80")

# disable UART in/out, enable USB in/out
pico_enable_stdio_uart(picosim 0)
pico_enable_stdio_usb(picosim 1)
