# a simple test case
project (PreOrder)
set(CMAKE_IGNORE_DEPENDENCIES_ORDERING 1)
add_subdirectory(Library)
add_executable (PreOrder simple.cxx)
target_link_libraries(PreOrder simpleLib)
