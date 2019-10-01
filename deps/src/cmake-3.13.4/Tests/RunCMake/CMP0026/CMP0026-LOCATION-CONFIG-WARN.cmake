
enable_language(CXX)

add_library(somelib empty.cpp)
get_target_property(_loc somelib LOCATION_Debug)
