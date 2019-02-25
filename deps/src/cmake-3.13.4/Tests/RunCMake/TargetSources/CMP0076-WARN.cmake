cmake_minimum_required(VERSION 3.12)

add_library(publiclib)

add_subdirectory(CMP0076-WARN)

add_executable(main main.cpp)
target_link_libraries(main publiclib)
