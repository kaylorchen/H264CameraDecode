cmake_minimum_required(VERSION 3.5)
project(Decode_H264)

set(CMAKE_CXX_STANDARD 11)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

add_executable(Decode_H264 main.cpp)

target_link_libraries(Decode_H264 avcodec avformat avutil ${SDL2_LIBRARIES} swscale)