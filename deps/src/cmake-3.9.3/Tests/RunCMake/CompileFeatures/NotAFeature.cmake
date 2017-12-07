
add_library(somelib STATIC empty.cpp)
set_property(TARGET somelib PROPERTY COMPILE_FEATURES "not_a_feature")
