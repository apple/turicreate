cmake_minimum_required(VERSION 2.8)

project(giomm CXX)

find_package(GTK2 COMPONENTS gtk gtkmm REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(giomm WIN32 main.cpp)
target_link_libraries(giomm GTK2::giomm)
