
set(CMAKE_DEBUG_TARGET_PROPERTIES COMPILE_FEATURES)
add_library(somelib STATIC empty.cpp)
target_compile_features(somelib PRIVATE not_a_feature)
