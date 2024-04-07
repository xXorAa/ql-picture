cmake_minimum_required(VERSION 3.16)
project(ql-pictures)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(ImageMagick REQUIRED COMPONENTS MagickWand)

if(${ImageMagick_VERSION_STRING} VERSION_LESS "7.0.0")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DMAGICK_V6=1")
endif()

list(JOIN ImageMagick_DEFINITIONS " " IM_DEFS)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${IM_DEFS}")

include_directories(${ImageMagick_INCLUDE_DIRS} include/)

add_executable(ql-picture
    src/ql-palette.c
    src/ql-picture.c
    )

target_link_libraries(ql-picture ${ImageMagick_LIBRARIES})

