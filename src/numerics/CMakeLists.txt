project(numerics)

make_empty_library(numerics REQUIRES armadillo serialization)
target_include_directories(numerics PUBLIC ${CMAKE_BINARY_DIR}/src/external/armadillo/tmp/include)

