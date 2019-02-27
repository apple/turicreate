cmake_minimum_required(VERSION 3.10)
project(Parallel2)
include("../AutogenTest.cmake")

# Test different values for AUTOGEN_PARALLEL
include("../Parallel/parallel.cmake")

add_executable(parallel2 ${PARALLEL_SRC})
set_target_properties(parallel2 PROPERTIES AUTOGEN_PARALLEL 2)
target_link_libraries(parallel2 ${QT_LIBRARIES})
