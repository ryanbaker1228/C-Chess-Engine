cmake_minimum_required(VERSION 3.21)
project(Chess_Engine)

set(CMAKE_CXX_STANDARD 23)

set(SDL_INCLUDE_DIR "/usr/include/SDL2")

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})
link_directories(${SDL2_LIBRARY_DIRS})
add_definitions(${SDL2_DEFINITIONS})

find_package(SDL2_IMAGE REQUIRED)
include_directories(${SDL2_IMAGE_INCLUDE_DIRS})
link_directories(${SDL2_IMAGE_LIBRARY_DIRS})
add_definitions(${SDL2_IMAGE_DEFINITIONS})

set(SOURCES
        main.cpp
        gui.cpp gamestate.h gamestate.cpp movegen.cpp movegen.h move.h evaluation.h evaluation.cpp move.cpp bitUtils.h bitUtils.cpp Search.h Search.cpp Test.h Test.cpp)

add_executable(Chess_Engine ${SOURCES})

target_link_libraries(Chess_Engine SDL2::SDL2 SDL2::SDL2main SDL2_image::SDL2_image)

set(GCC_COVERAGE_COMPILE_FLAGS "-lSDL2")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}" )