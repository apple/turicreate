enable_language(CXX)
add_library(foo foo.cpp)
target_link_libraries(foo ${CMAKE_BINARY_DIR}/xyzzy.targets)

add_library(bar bar.cpp)
target_link_libraries(bar foo ${CMAKE_BINARY_DIR}/waldo.targets)

add_executable(baz baz.cpp)
target_link_libraries(baz bar)
