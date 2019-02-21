enable_language(CXX)
add_library(all empty.cpp)
add_executable(clean empty.cpp)
add_custom_target(help)
