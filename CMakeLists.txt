cmake_minimum_required(VERSION 3.16)  # Increased minimum version for better Qt support

project(dslgui VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Try to find Qt6 first, fall back to Qt5
find_package(Qt6 QUIET COMPONENTS Core Widgets OpenGLWidgets)
if(Qt6_FOUND)
    message(STATUS "Using Qt6")
    set(QT_VERSION_MAJOR 6)
else()
    find_package(Qt5 5.15 REQUIRED COMPONENTS Core Widgets OpenGL)
    message(STATUS "Using Qt5")
    set(QT_VERSION_MAJOR 5)
endif()

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
)

add_executable(dslgui ${PROJECT_SOURCES}
    lexer.h lexer.cpp
    parser.h parser.cpp
    parser_rules.h
    sema.h sema.cpp
    translation.h translation.cpp
)

if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(dslgui PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::OpenGLWidgets
    )
else()
    target_link_libraries(dslgui PRIVATE
        Qt5::Core
        Qt5::Widgets
        Qt5::OpenGL
    )
endif()

# Set target properties
set_target_properties(dslgui PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS dslgui
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
