
enable_language(CXX)

add_library(testTarget "${CMAKE_CURRENT_SOURCE_DIR}/empty.cpp")

add_library(imported UNKNOWN IMPORTED)
set_property(TARGET imported PROPERTY INTERFACE_INCLUDE_DIRECTORIES "/does/not/exist")

target_link_libraries(testTarget imported)
