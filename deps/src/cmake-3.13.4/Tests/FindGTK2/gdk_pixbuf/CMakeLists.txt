cmake_minimum_required(VERSION 2.8)

project(gdk_pixbuf C)

find_package(GTK2 COMPONENTS gtk REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(gdk_pixbuf WIN32 main.c)
target_link_libraries(gdk_pixbuf GTK2::gdk_pixbuf)
