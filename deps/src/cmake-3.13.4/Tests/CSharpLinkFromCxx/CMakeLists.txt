# Take a C# shared library and link it to a managed C++ shared library
cmake_minimum_required(VERSION 3.10)
project (CSharpLinkFromCxx CXX CSharp)

add_library(CSharpLibrary SHARED UsefulCSharpClass.cs)

# we have to change the default flags for the
# managed C++ project to build
string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})

# The C# project is a dependency of the C++/CLI project
add_library(ManagedCppLibrary SHARED UsefulManagedCppClass.cpp UsefulManagedCppClass.hpp)
target_compile_options(ManagedCppLibrary PRIVATE "/clr")
target_link_libraries(ManagedCppLibrary PUBLIC CSharpLibrary)

# Main executable for the test framework
add_executable(CSharpLinkFromCxx CSharpLinkFromCxx.cs)
target_link_libraries(CSharpLinkFromCxx PRIVATE ManagedCppLibrary)
