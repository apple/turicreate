
add_library(foo empty.cpp)
set_property(TARGET foo PROPERTY CXX_STANDARD 11)
set_property(TARGET foo PROPERTY CXX_STANDARD_REQUIRED TRUE)
