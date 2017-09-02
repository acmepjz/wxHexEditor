project(wxHexEditor)

cmake_minimum_required(VERSION 2.8)

# Set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

if(WIN32)
	add_definitions (-D__const=)
endif()

add_subdirectory(mhash)
add_subdirectory(udis86)
add_subdirectory(src)
