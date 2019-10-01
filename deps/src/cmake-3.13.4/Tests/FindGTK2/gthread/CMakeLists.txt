cmake_minimum_required(VERSION 2.8)

project(gthread C)

find_package(GTK2 COMPONENTS gtk REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(gthread WIN32 main.c)
target_link_libraries(gthread GTK2::gthread)
