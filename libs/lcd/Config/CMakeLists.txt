add_library(lcd-Config INTERFACE)

target_include_directories(lcd-Config INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(lcd-Config INTERFACE
	pico_stdlib
	hardware_pwm
	hardware_spi
	hardware_sync
)

target_sources(lcd-Config INTERFACE
	DEV_Config.c
)
