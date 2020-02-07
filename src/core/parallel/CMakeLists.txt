project(Turi)

make_library(
  parallel
  OBJECT
  SOURCES
  pthread_tools.cpp
  thread_pool.cpp
  execute_task_in_native_thread.cpp
  lambda_omp.cpp
  REQUIRES
  platform_config
  logger
  EXTERNAL_VISIBILITY)

if(NOT ${TC_BUILD_IOS})
  # get_target_property(TURI_OMP_H_PATH omp TURI_OMP_H_PATH) if
  # (TURI_OMP_H_PATH) message(FATAL_ERROR "TURI_OMP_H_PATH not set in target
  # omp") endif() set_source_files_properties(lambda_omp.cpp PROPERTIES
  # COMPILE_FLAGS -I${TURI_OMP_H_PATH})
  target_link_libraries(parallel omp)
  set_source_files_properties(lambda_omp.hpp PROPERTIES COMPILE_FLAGS -fopenmp)
  # target_link_libraries(lambda_omp.cpp omp)
endif()
