if(WIN32)
	set(wxWidgets_CONFIGURATION mswu)
else()
	set(wxWidgets_USE_UNICODE ON)
endif()

# find_package(wxWidgets REQUIRED adv aui core gl html media propgrid qa ribbon richtext stc webview xrc base)
find_package(wxWidgets REQUIRED adv aui core net xml base)

include_directories(${wxWidgets_INCLUDE_DIRS})
include_directories(../mhash/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../mhash/include)
include_directories(../udis86)
include_directories(../udis86/libudis86)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../udis86)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../udis86/libudis86)

set (SOURCES
	FAL.cpp
	HexDialogs.cpp
	HexEditor.cpp
	HexEditorApp.cpp
	HexEditorCtrl/HexEditorCtrl.cpp
	HexEditorCtrl/HexEditorCtrlGui.cpp
	HexEditorCtrl/wxHexCtrl/Tag.cpp
	HexEditorCtrl/wxHexCtrl/TagDialogGui.cpp
	HexEditorCtrl/wxHexCtrl/wxHexCtrl.cpp
	HexEditorFrame.cpp
	HexEditorGui.cpp
	HexPanels.cpp
)

set (HEADERS
	FAL.h
	HexDialogs.h
	HexEditor.h
	HexEditorApp.h
	HexEditorCtrl/HexEditorCtrl.h
	HexEditorCtrl/HexEditorCtrlGui.h
	HexEditorCtrl/wxHexCtrl/Tag.h
	HexEditorCtrl/wxHexCtrl/TagDialogGui.h
	HexEditorCtrl/wxHexCtrl/wxHexCtrl.h
	HexEditorFrame.h
	HexEditorGui.h
	HexPanels.h
)

if(WIN32)
	set(SOURCES ${SOURCES} windrv.cpp ../resources/resource.rc)
	set(HEADERS ${HEADERS} windrv.h)
	add_definitions(-DWXUSINGDLL)
endif(WIN32)

add_executable(wxHexEditor WIN32
	${SOURCES} ${HEADERS}
)

target_link_libraries(wxHexEditor
	${wxWidgets_LIBRARIES}
	mhash udis86
)
