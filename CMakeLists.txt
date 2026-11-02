cmake_minimum_required(VERSION 3.21)

project(EagleHub VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

add_executable(EagleHub WIN32
    src/main.cpp
    assets/EagleHub.qrc
    assets/EagleHub.rc
)

target_link_libraries(EagleHub
    PRIVATE
        Qt6::Widgets
        Qt6::Network
)

install(TARGETS EagleHub
    RUNTIME DESTINATION .
)
