cmake_minimum_required(VERSION 4.2)
project(voxelcraft C)

set(CMAKE_C_STANDARD 23)

include(FetchContent)

FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG        5.5
        GIT_SHALLOW    TRUE
)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(raylib)

add_executable(${PROJECT_NAME} src/main.c
        src/chunk.h
        src/block.h
        src/raycast.h
        src/world.h)

target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
