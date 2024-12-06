add_library(lcd-LCD INTERFACE)

target_include_directories(lcd-LCD INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(lcd-LCD INTERFACE
	lcd-Config
)

target_sources(lcd-LCD INTERFACE
	LCD_1in14_V2.c
)
