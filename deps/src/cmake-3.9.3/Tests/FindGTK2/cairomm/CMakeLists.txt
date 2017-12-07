cmake_minimum_required(VERSION 2.8)

project(cairomm CXX)

find_package(GTK2 COMPONENTS gtk gtkmm REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(cairomm WIN32 main.cpp)
target_link_libraries(cairomm GTK2::cairomm)
