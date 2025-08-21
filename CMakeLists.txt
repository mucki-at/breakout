cmake_minimum_required(VERSION 3.10.0)
project(breakout VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_VERBOSE_MAKEFILE NO)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS NO)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)

add_executable(breakout
    main.cpp
    game.cpp
    )

target_link_libraries(${PROJECT_NAME} SDL2::SDL2 Vulkan::Vulkan)