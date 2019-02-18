
enable_language(CXX)

add_subdirectory(dep1)
add_subdirectory(dep2)
add_subdirectory(dep3)

add_library(somelib empty.cpp)
target_link_libraries(somelib dep3)
