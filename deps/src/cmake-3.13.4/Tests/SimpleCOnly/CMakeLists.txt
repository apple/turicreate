# this enables only C, i.e. disables C++
project(SimpleCOnly C)

add_library(SimpleCLib STATIC bar.c foo.c)

add_executable(SimpleC main.c)
target_link_libraries(SimpleC SimpleCLib)

# and some check, just to make sure it works:
include(CheckTypeSize)
check_type_size(float SIZE_FLOAT)
message(STATUS "sizeof(float): ${SIZE_FLOAT}")

# make sure optimized libs are not used by debug builds
if(CMAKE_BUILD_TYPE MATCHES Debug)
  target_link_libraries(Simple optimized c:/not/here.lib )
endif()
