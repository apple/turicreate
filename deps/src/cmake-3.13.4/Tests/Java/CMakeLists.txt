project(hello Java)

cmake_minimum_required (VERSION 2.6)
set(CMAKE_VERBOSE_MAKEFILE 1)

find_package(Java COMPONENTS Development)
include (UseJava)

add_jar(hello A.java HelloWorld.java)

# use listing file to specify sources
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/java_fileslist "A.java\nHelloWorld.java\n")
add_jar(hello2 @${CMAKE_CURRENT_BINARY_DIR}/java_fileslist)

# use listing file to specify sources and specify output directory (issue #17316)
add_jar(hello3 @${CMAKE_CURRENT_BINARY_DIR}/java_fileslist OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/hello3")
