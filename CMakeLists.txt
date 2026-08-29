cmake_minimum_required(VERSION 3.16)
project(FitToList-Cpp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Test)

qt_standard_project_setup()

# UI-free, unit-testable image/file-naming logic, shared by the app and tests.
add_library(FitToListCore STATIC
    src/FileNaming.h
    src/FileNaming.cpp
    src/ImageDocument.h
    src/ImageDocument.cpp
    src/Config.h
    src/Config.cpp
)
target_include_directories(FitToListCore PUBLIC src)
target_link_libraries(FitToListCore PUBLIC Qt6::Widgets)

# Reusable widgets, built on FitToListCore.
add_library(FitToListUi STATIC
    src/CanvasWidget.h
    src/CanvasWidget.cpp
    src/DirectoryTreeWidget.h
    src/DirectoryTreeWidget.cpp
    src/FolderSelectionDialog.h
    src/FolderSelectionDialog.cpp
    src/MaxSizeDialog.h
    src/MaxSizeDialog.cpp
    src/ImageTreeWidget.h
    src/ImageTreeWidget.cpp
)
target_include_directories(FitToListUi PUBLIC src)
target_link_libraries(FitToListUi PUBLIC FitToListCore Qt6::Widgets)

qt_add_executable(FitToList
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
)

target_link_libraries(FitToList PRIVATE FitToListUi FitToListCore Qt6::Widgets)

enable_testing()
add_subdirectory(tests)
