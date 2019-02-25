
set(CMAKE_DEBUG_TARGET_PROPERTIES COMPILE_FEATURES)
add_library(somelib STATIC empty.cpp)
set_property(TARGET somelib PROPERTY COMPILE_FEATURES "$<1:not_a_feature>")
