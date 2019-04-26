cmake_minimum_required(VERSION 3.10)
project(ObjectLibrary)
include("../AutogenTest.cmake")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

# Object library a defined in a subdirectory
add_subdirectory(a)

# Object library b defined locally
include_directories(b)
add_library(b OBJECT b/classb.cpp)
target_compile_features(b PRIVATE ${QT_COMPILE_FEATURES})

# Executable with OBJECT library generator expressions
add_executable(someProgram main.cpp $<TARGET_OBJECTS:a> $<TARGET_OBJECTS:b>)
target_link_libraries(someProgram ${QT_LIBRARIES})
