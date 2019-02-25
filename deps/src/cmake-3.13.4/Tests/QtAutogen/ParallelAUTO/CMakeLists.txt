cmake_minimum_required(VERSION 3.10)
project(ParallelAUTO)
include("../AutogenTest.cmake")

# Test different values for AUTOGEN_PARALLEL
include("../Parallel/parallel.cmake")

add_executable(parallelAUTO ${PARALLEL_SRC})
set_target_properties(parallelAUTO PROPERTIES AUTOGEN_PARALLEL "AUTO")
target_link_libraries(parallelAUTO ${QT_LIBRARIES})
