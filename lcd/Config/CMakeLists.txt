# Find all source files in the current directory
# and save their names in the DIR_Config_SRCS variable
aux_source_directory(. DIR_Config_SRCS)

# Generate link library
add_library(Config ${DIR_Config_SRCS})
target_link_libraries(Config PUBLIC pico_stdlib hardware_spi hardware_i2c hardware_pwm hardware_adc)
