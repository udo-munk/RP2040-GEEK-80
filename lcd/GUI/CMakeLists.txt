# Find all source files in the current directory
# and save their names in the DIR_GUI_SRCS variable
aux_source_directory(. DIR_GUI_SRCS)

include_directories(../Config)

# Generate link library
add_library(GUI ${DIR_GUI_SRCS})
target_link_libraries(GUI PUBLIC Config)
