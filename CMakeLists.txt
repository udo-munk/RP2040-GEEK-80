cmake_minimum_required(VERSION 3.12)
set (PICO_BOARD pico)
include(pico_sdk_import.cmake)

project(picosim C CXX ASM)
pico_sdk_init()

include_directories(
	.
	../z80pack/z80core
	../z80pack/iodevices
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
)
add_subdirectory(
	no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/src build
)
target_compile_definitions(picosim PRIVATE
	PICO_STACK_SIZE=4096
	PICO_CORE1_STACK_SIZE=4096
	PICO_HEAP_SIZE=8192
)
target_compile_definitions(picosim INTERFACE
	PICO_DEFAULT_UART=0
)

target_link_libraries(picosim
	pico_stdlib
	no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
)

pico_add_extra_outputs(picosim)

pico_set_program_name(picosim "z80pack picosim")
pico_set_program_version(picosim "1.2")

# disable UART in/out, enable USB in/out
pico_enable_stdio_uart(picosim 0)
pico_enable_stdio_usb(picosim 1)
