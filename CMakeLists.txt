cmake_minimum_required(VERSION 3.16)
project(FitToList-Cpp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(FitToList
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
)

target_link_libraries(FitToList PRIVATE Qt6::Widgets)
