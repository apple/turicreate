
add_library(no_features empty.cpp)
target_compile_features(no_features PRIVATE cxx_constexpr)
