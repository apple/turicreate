
enable_language(CXX)

add_library(foo SHARED empty.cpp)
set_target_properties(foo PROPERTIES LINK_DIRECTORIES "../lib")
