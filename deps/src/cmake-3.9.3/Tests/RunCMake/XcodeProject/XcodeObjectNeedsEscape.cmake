enable_language(CXX)
string(APPEND CMAKE_CXX_FLAGS " -DKDESRCDIR=\\\"foo\\\"")
add_library(foo STATIC foo.cpp)
