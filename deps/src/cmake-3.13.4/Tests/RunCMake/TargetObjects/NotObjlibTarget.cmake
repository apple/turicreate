add_library(StaticLib empty.cpp)

file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/test_output CONTENT $<TARGET_OBJECTS:StaticLib>)
