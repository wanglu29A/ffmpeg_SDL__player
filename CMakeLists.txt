cmake_minimum_required(VERSION 3.10)
project(ffmpeg_SDL__player C)

set(CMAKE_C_STANDARD 99)
link_directories(lib)
include_directories(include)
add_executable(ffmpeg_SDL_player main.c)
target_link_libraries(ffmpeg_SDL_player -lavcodec -lavdevice -lavfilter -lavfilter
                       -lavformat -lavutil -lswresample -lswscale -lSDL2)