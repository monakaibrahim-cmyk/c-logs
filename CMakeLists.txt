cmake_minimum_required(VERSION 3.10)
project(Logs C)

set(CMAKE_C_STANDARD 11)

add_library(Logs STATIC src/Logs/Logs.c)

target_include_directories(Logs PUBLIC src/Logs)

find_package(Threads REQUIRED)
target_link_libraries(Logs PUBLIC Threads::Threads m)
