# test if CSharp application correctly links
# to managed C++ binary
cmake_minimum_required(VERSION 3.9)
project (CSharpLinkToCxx CXX CSharp)

# we have to change the default flags for the
# managed C++ project to build
string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})

add_library(CLIApp SHARED cli.hpp cli.cpp)

target_compile_options(CLIApp PRIVATE "/clr")

add_executable(CSharpLinkToCxx csharp.cs)

target_link_libraries(CSharpLinkToCxx CLIApp)

# this unmanaged C++ library will be added to the C#/.NET
# references of CSharpLinkToCxx but it will show a warning
# because it is unmanaged
add_library(CppNativeApp SHARED cpp_native.hpp cpp_native.cpp)
target_link_libraries(CSharpLinkToCxx CppNativeApp)
