
enable_language(CXX)

add_library(foo OBJECT empty.cpp)
add_executable(bar $<TARGET_OBJECTS:foo>)
get_target_property(location bar LOCATION)
