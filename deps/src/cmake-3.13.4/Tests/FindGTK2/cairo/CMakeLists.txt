cmake_minimum_required(VERSION 2.8)

project(cairo C)

find_package(GTK2 COMPONENTS gtk REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(cairo WIN32 main.c)
target_link_libraries(cairo GTK2::cairo)
