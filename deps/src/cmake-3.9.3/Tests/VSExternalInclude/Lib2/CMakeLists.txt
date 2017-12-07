set(CMAKE_SUPPRESS_REGENERATION 1)
project(VSEXTERNAL_LIB2)

include_directories(${VSEXTERNAL_LIB2_SOURCE_DIR}/../Lib1)

set(SOURCES lib2.cpp)

add_library(lib2 ${SOURCES})
