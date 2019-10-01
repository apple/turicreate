
enable_language(CXX)

cmake_policy(SET CMP0027 OLD)

add_library(testTarget UNKNOWN IMPORTED)
set_property(TARGET testTarget PROPERTY INTERFACE_INCLUDE_DIRECTORIES "/does/not/exist")

add_library(userTarget "${CMAKE_CURRENT_SOURCE_DIR}/empty.cpp")
target_link_libraries(userTarget PRIVATE $<1:testTarget>)
