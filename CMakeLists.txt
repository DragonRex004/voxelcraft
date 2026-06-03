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
        src/world.h
        src/block.c
        src/chunk.c
        src/atlas.h
        src/atlas.c
        src/block_loader.h
        src/cJson/cJSON.c
        src/cJson/cJSON_Utils.c)

target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

add_custom_target(copy_assets ALL
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "${CMAKE_BINARY_DIR}/assets"
        COMMENT "Copy Assets in Build-Directory..."
)

add_dependencies(${PROJECT_NAME} copy_assets)
