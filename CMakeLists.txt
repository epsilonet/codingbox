cmake_minimum_required(VERSION 3.21)

project(Codingbox
  VERSION 0.1.0
  DESCRIPTION "A focused native desktop IDE"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(CODINGBOX_SOURCES
  src/main.cpp
  src/MainWindow.cpp
  src/MainWindow.h
  src/CodeEditor.cpp
  src/CodeEditor.h
  src/SyntaxHighlighter.cpp
  src/SyntaxHighlighter.h
)

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
  qt_add_executable(Codingbox MANUAL_FINALIZATION ${CODINGBOX_SOURCES})
else()
  add_executable(Codingbox ${CODINGBOX_SOURCES})
endif()

target_link_libraries(Codingbox PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

target_compile_definitions(Codingbox PRIVATE
  CODINGBOX_VERSION="${PROJECT_VERSION}"
)

set_target_properties(Codingbox PROPERTIES
  WIN32_EXECUTABLE TRUE
  MACOSX_BUNDLE TRUE
  MACOSX_BUNDLE_GUI_IDENTIFIER "io.codingbox.desktop"
  MACOSX_BUNDLE_BUNDLE_NAME "Codingbox"
  MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
  MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
)

include(GNUInstallDirs)
install(TARGETS Codingbox
  BUNDLE DESTINATION .
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
  qt_finalize_executable(Codingbox)
endif()
