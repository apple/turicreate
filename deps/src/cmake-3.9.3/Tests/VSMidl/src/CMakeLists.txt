cmake_minimum_required(VERSION 2.8)
project(VSMidl)

include_directories("${CMAKE_CURRENT_BINARY_DIR}/\$(IntDir)")

add_executable(VSMidl main.cpp test.idl)
