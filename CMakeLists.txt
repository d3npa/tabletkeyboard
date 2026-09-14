cmake_minimum_required(VERSION 3.16)
project(tabletkeyboard VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(CTest)

find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui Widgets Network)
find_package(X11 REQUIRED)
find_library(XTST_LIBRARY NAMES Xtst)
if (NOT XTST_LIBRARY)
    message(FATAL_ERROR "libXtst not found: install the Xtst development package")
endif()

# --- osk-core: pure logic, QtCore only, no X11, no widgets -----------------
add_library(osk_core STATIC
    src/core/action.h
    src/core/keysyms.h
    src/core/keysyms.cpp
    src/core/keystate.h
    src/core/keystate.cpp
    src/core/layout.h
    src/core/layout.cpp
    src/core/geometry.h
    src/core/geometry.cpp
    src/core/lint.h
    src/core/lint.cpp
    src/core/sym_resolver.h
    src/core/theme.h
    src/core/theme.cpp
)
target_include_directories(osk_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(osk_core PUBLIC Qt5::Core)

# --- osk-platform: Xlib + XTest --------------------------------------------
add_library(osk_platform STATIC
    src/platform/input_backend.h
    src/platform/window_adapter.h
    src/platform/x11/input_xtest.h
    src/platform/x11/input_xtest.cpp
    src/platform/x11/keycodes.h
    src/platform/x11/keycodes.cpp
    src/platform/x11/symresolver_x11.h
    src/platform/x11/symresolver_x11.cpp
    src/platform/x11/window_x11.h
    src/platform/x11/window_x11.cpp
    src/platform/x11/xconnection.h
    src/platform/x11/xconnection.cpp
)
target_link_libraries(osk_platform PUBLIC osk_core X11::X11 ${XTST_LIBRARY})

# --- osk-ui: Qt5 Widgets ---------------------------------------------------
add_library(osk_ui STATIC
    src/ui/keyboardwindow.h
    src/ui/keyboardwindow.cpp
    src/ui/keybutton.h
    src/ui/keybutton.cpp
    src/ui/themepainter.h
    src/ui/themepainter.cpp
    src/ui/titlebar.h
    src/ui/titlebar.cpp
)
target_link_libraries(osk_ui PUBLIC osk_core Qt5::Widgets Qt5::Gui)

# --- tabletkeyboard --------------------------------------------------------
add_executable(tabletkeyboard
    src/main.cpp
    src/app/app.h
    src/app/app.cpp
    src/app/autostart.h
    src/app/autostart.cpp
    src/app/icon.h
    src/app/icon.cpp
    src/app/settings.h
    src/app/settings.cpp
    src/app/singleinstance.h
    src/app/singleinstance.cpp
    src/app/tray.h
    src/app/tray.cpp
    data/tabletkeyboard.qrc
)
target_compile_definitions(tabletkeyboard PRIVATE TABLETKEYBOARD_VERSION="${PROJECT_VERSION}")
target_link_libraries(tabletkeyboard PRIVATE osk_core osk_platform osk_ui Qt5::Widgets Qt5::Network)

# --- install ---------------------------------------------------------------
include(GNUInstallDirs)
install(TARGETS tabletkeyboard RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES data/tabletkeyboard.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES data/icons/tabletkeyboard.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
install(DIRECTORY data/layouts/ DESTINATION ${CMAKE_INSTALL_DATADIR}/tabletkeyboard/layouts)
install(DIRECTORY data/themes/ DESTINATION ${CMAKE_INSTALL_DATADIR}/tabletkeyboard/themes)

# --- tests -----------------------------------------------------------------
if (BUILD_TESTING)
    find_package(Qt5 COMPONENTS Test QUIET)
    if (NOT TARGET Qt5::Test)
        message(STATUS "Qt5 Test component not found: unit tests disabled")
    else()
        foreach(test_name tst_layout tst_geometry tst_keystate tst_theme)
            add_executable(${test_name} tests/${test_name}.cpp data/tabletkeyboard.qrc)
            target_compile_definitions(${test_name} PRIVATE
                TABLETKEYBOARD_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")
            target_link_libraries(${test_name} PRIVATE osk_core osk_platform Qt5::Test)
            add_test(NAME ${test_name} COMMAND ${test_name})
        endforeach()
    endif()
endif()
