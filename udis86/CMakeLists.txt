include_directories(libudis86)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/libudis86)

find_package(PythonInterp REQUIRED)

configure_file (docs/x86/optable.xml libudis86/optable.xml COPYONLY)
exec_program(${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/libudis86
	ARGS ${CMAKE_CURRENT_SOURCE_DIR}/scripts/ud_itab.py optable.xml ./
)

set (SOURCES
	libudis86/decode.c
	${CMAKE_CURRENT_BINARY_DIR}/libudis86/itab.c
	libudis86/syn-att.c
	libudis86/syn-intel.c
	libudis86/syn.c
	libudis86/udis86.c
)

set (HEADERS
	udis86.h
	libudis86/syn.h
	libudis86/types.h
	libudis86/udint.h
	libudis86/decode.h
	libudis86/extern.h
	${CMAKE_CURRENT_BINARY_DIR}/libudis86/itab.h
)

add_definitions(-DHAVE_STRING_H)

add_library(udis86 STATIC ${SOURCES} ${HEADERS})
