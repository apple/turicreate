
cmake_policy(SET CMP0028 OLD)

add_library(foo empty.cpp)
target_link_libraries(foo PRIVATE External::Library)
