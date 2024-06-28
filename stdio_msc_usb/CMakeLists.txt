if (TARGET tinyusb_device_unmarked)
    pico_add_library(stdio_msc_usb)

    target_include_directories(stdio_msc_usb_headers INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

    target_sources(stdio_msc_usb INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/reset_interface.c
        ${CMAKE_CURRENT_LIST_DIR}/stdio_msc_usb.c
        ${CMAKE_CURRENT_LIST_DIR}/stdio_msc_usb_descriptors.c
    )

    pico_mirrored_target_link_libraries(stdio_msc_usb INTERFACE
        pico_stdio
        pico_time
        pico_unique_id
        pico_usb_reset_interface
    )
    target_link_libraries(stdio_msc_usb INTERFACE
        tinyusb_device_unmarked
    )
    # PICO_CMAKE_CONFIG: PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS, Maximum number of milliseconds to wait during initialization for a CDC connection from the host (negative means indefinite) during initialization, default=0, group=pico_stdio_usb
    if (PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS)
        target_compile_definitions(stdio_msc_usb INTERFACE
            PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=${PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS}
        )
    endif()
endif()
