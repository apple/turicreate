enable_language(CXX)

cmake_policy(SET CMP0021 NEW)

add_library(testTarget "${CMAKE_CURRENT_SOURCE_DIR}/empty.cpp")
set_property(TARGET testTarget PROPERTY INTERFACE_INCLUDE_DIRECTORIES "$<1:foo>")

add_library(userTarget "${CMAKE_CURRENT_SOURCE_DIR}/empty.cpp")
target_include_directories(userTarget PRIVATE $<TARGET_PROPERTY:testTarget,INTERFACE_INCLUDE_DIRECTORIES>)
