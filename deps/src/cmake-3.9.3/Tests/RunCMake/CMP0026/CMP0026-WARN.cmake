
enable_language(CXX)

add_library(somelib empty.cpp)
get_target_property(_loc somelib LOCATION)

add_subdirectory(CMP0026-WARN-Dir)
get_target_property(_loc otherlib LOCATION)
