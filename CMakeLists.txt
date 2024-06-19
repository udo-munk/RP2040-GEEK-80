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
)

target_compile_definitions(picosim INTERFACE
	PICO_DEFAULT_UART=0
)

target_compile_options(picosim PUBLIC
	### GNU/Clang C Options
	$<$<COMPILE_LANG_AND_ID:C,GNU>:-fdiagnostics-color=always>
	$<$<COMPILE_LANG_AND_ID:C,Clang>:-fcolor-diagnostics>

	$<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Wall>
	$<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Wextra>
	$<$<COMPILE_LANG_AND_ID:C,Clang>:-Weverything>
	$<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Wno-unused-parameter>
	)

target_link_libraries(picosim
	pico_stdlib
	no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
	LCD
	GUI
	Fonts
	Config
	hardware_spi
)

pico_add_extra_outputs(picosim)

set_property(TARGET picosim APPEND_STRING PROPERTY LINK_FLAGS "-Wl,--print-memory-usage")

pico_set_program_name(picosim "z80pack picosim")
pico_set_program_description(picosim "z80pack on Waveshare RP2040-GEEK")
pico_set_program_version(picosim "1.2")
pico_set_program_url(picosim "https://github.com/udo-munk/RP2040-GEEK-80")

# disable UART in/out, enable USB in/out
pico_enable_stdio_uart(picosim 0)
pico_enable_stdio_usb(picosim 1)
