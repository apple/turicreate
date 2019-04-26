cmake_minimum_required(VERSION 2.8)

project(pangocairo C)

find_package(GTK2 COMPONENTS gtk REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(pangocairo WIN32 main.c)
target_link_libraries(pangocairo GTK2::pangocairo m)
