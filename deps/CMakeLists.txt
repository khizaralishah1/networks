cmake_minimum_required(VERSION 3.14)
project(learn_libuv C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_subdirectory(libuv)

add_executable(engine demos/Main.c)
target_link_libraries(engine PRIVATE uv)