cmake_minimum_required(VERSION 3.10)
project(Parallel3)
include("../AutogenTest.cmake")

# Test different values for AUTOGEN_PARALLEL
include("../Parallel/parallel.cmake")

add_executable(parallel3 ${PARALLEL_SRC})
set_target_properties(parallel3 PROPERTIES AUTOGEN_PARALLEL 3)
target_link_libraries(parallel3 ${QT_LIBRARIES})
