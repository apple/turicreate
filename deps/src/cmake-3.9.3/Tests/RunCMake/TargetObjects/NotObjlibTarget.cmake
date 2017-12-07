add_library(StaticLib empty.cpp)

file(GENERATE OUTPUT test_output CONTENT $<TARGET_OBJECTS:StaticLib>)
