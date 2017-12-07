
add_library(foo empty.cpp)
target_link_libraries(foo PRIVATE External::Library)
