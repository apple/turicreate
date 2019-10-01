cmake_minimum_required(VERSION 3.10)
project(Parallel1)
include("../AutogenTest.cmake")

# Test different values for AUTOGEN_PARALLEL
include("../Parallel/parallel.cmake")

add_executable(parallel1 ${PARALLEL_SRC})
set_target_properties(parallel1 PROPERTIES AUTOGEN_PARALLEL 1)
target_link_libraries(parallel1 ${QT_LIBRARIES})
