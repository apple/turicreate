cmake_minimum_required(VERSION 2.8)

project(gtkmm CXX)

find_package(GTK2 COMPONENTS gtk gtkmm REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_executable(gtkmm-target WIN32 main.cpp helloworld.cpp helloworld.h)
target_link_libraries(gtkmm-target GTK2::gtkmm)

add_executable(gtkmm-all-libs WIN32 main.cpp helloworld.cpp helloworld.h)
target_link_libraries(gtkmm-all-libs ${GTK2_LIBRARIES})
target_include_directories(gtkmm-all-libs PRIVATE ${GTK2_INCLUDE_DIRS})

# Linking via the library variables does not cause compile feature
# requirements to propagate.  Do it manually for purposes of this test.
get_property(features TARGET GTK2::sigc++ PROPERTY INTERFACE_COMPILE_FEATURES)
set_property(TARGET gtkmm-all-libs PROPERTY COMPILE_FEATURES ${features})
