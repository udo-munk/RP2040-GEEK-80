add_library(lcd-Fonts INTERFACE)

target_include_directories(lcd-Fonts INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(lcd-Fonts INTERFACE
	pico_platform
)

target_sources(lcd-Fonts INTERFACE
	font12.c
	font14.c
	font16.c
	font18.c
	font20.c
	font22.c
	font24.c
	font28.c
	font32.c
)
