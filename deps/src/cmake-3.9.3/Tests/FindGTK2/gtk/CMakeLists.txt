cmake_minimum_required(VERSION 2.8)

project(gtk C)

find_package(GTK2 COMPONENTS gtk REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(gtk WIN32 main.c)
target_link_libraries(gtk GTK2::gtk)

add_executable(gtk-all-libs WIN32 main.c)
target_link_libraries(gtk-all-libs ${GTK2_LIBRARIES})
target_include_directories(gtk-all-libs PRIVATE ${GTK2_INCLUDE_DIRS})
