cmake_minimum_required(VERSION 3.10)
project(Parallel)
include("../AutogenTest.cmake")

# Test different values for AUTOGEN_PARALLEL
include("../Parallel/parallel.cmake")

add_executable(parallel ${PARALLEL_SRC})
set_target_properties(parallel PROPERTIES AUTOGEN_PARALLEL "")
target_link_libraries(parallel ${QT_LIBRARIES})
