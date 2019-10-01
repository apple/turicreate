
enable_language(CXX)

add_library(foo empty.cpp)

add_library(alias ALIAS foo)

target_include_directories(alias PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)
