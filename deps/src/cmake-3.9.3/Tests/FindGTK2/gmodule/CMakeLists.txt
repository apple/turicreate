cmake_minimum_required(VERSION 2.8)

project(gmodule C)

find_package(GTK2 COMPONENTS gtk REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(gmodule WIN32 main.c)
target_link_libraries(gmodule GTK2::gmodule)
