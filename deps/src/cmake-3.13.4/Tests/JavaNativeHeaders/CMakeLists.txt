project(helloJavaNativeHeaders Java CXX)

cmake_minimum_required (VERSION 2.6)
set(CMAKE_VERBOSE_MAKEFILE 1)

find_package(Java COMPONENTS Development)
include (UseJava)

# JNI support
find_package(JNI)

add_jar(B1 D.java GENERATE_NATIVE_HEADERS D1-native)
add_jar(E1 E.java GENERATE_NATIVE_HEADERS E1-native)

add_jar(hello4 HelloWorld3.java)

add_library(D SHARED D.cpp E.cpp)
target_link_libraries (D PRIVATE D1-native E1-native)
