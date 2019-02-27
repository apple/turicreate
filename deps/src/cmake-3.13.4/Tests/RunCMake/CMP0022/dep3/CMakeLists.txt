
link_libraries(dep1)

add_library(dep3 ../empty_vs6_3.cpp)
target_link_libraries(dep3 dep2)
