add_library(lcd-GUI INTERFACE)

target_include_directories(lcd-GUI INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(lcd-GUI INTERFACE
	lcd-Config
)

target_sources(lcd-GUI INTERFACE
	GUI_BmpWindows.c
	GUI_Clear.c
	GUI_ClearWindow.c
	GUI_DrawBitMap.c
	GUI_DrawBitMap_Block.c
	GUI_DrawChar.c
	GUI_DrawCircle.c
	GUI_DrawImage.c
	GUI_DrawImage1.c
	GUI_DrawLine.c
	GUI_DrawNum.c
	GUI_DrawPoint.c
	GUI_DrawRectangle.c
	GUI_DrawString.c
	GUI_DrawTime.c
	GUI_NewImage.c
	GUI_SelectImage.c
	GUI_SetDepth.c
	GUI_SetMirroring.c
	GUI_SetPixel.c
	GUI_SetRotate.c
)
