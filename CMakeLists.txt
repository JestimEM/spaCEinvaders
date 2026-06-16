cmake_minimum_required(VERSION 3.20)
project(spaceinvaders_cliente C)
set(CMAKE_C_STANDARD 11)

include_directories(client/include)

# --------------------------------------------------------------------------
# Raylib — ventana nativa, sin dependencia de consola Windows
# --------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

# --------------------------------------------------------------------------
# Ejecutable principal
# --------------------------------------------------------------------------
add_executable(spaceinvaders
    client/src/main.c
    client/src/network.c
    client/src/render.c
    client/src/json_utils.c
    client/src/alien.c
    client/src/player.c
    client/src/bunker.c
    client/src/bunker_visual.c
    client/src/game_state.c
    client/src/sprites.c
    client/src/efectos.c
)

target_include_directories(spaceinvaders PRIVATE client/include)

if(WIN32)
    target_link_libraries(spaceinvaders raylib ws2_32 winmm gdi32 opengl32)
else()
    target_link_libraries(spaceinvaders raylib m pthread)
endif()
