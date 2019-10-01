cmake_minimum_required(VERSION 3.5)
project(hello C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib-static")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

add_library(greeting SHARED greeting.c)
add_library(greeting2 STATIC greeting2.c)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_executable(hello hello_with_two_greetings.c)
target_link_libraries(hello greeting greeting2)

set(HELLO_OUTPUT_STRING "Hello world!\nHello world 2!\n")
include(CheckOutput.cmake)

include(CheckNoPrefixSubDir.cmake)
