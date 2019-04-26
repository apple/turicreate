set(CMAKE_XCODE_GENERATE_TOP_LEVEL_PROJECT_ONLY TRUE)
project(XcodeDependOnZeroCheck CXX)
add_subdirectory(zerocheck)
add_library(parentdirlib foo.cpp)
