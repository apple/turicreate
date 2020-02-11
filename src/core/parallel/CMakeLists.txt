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

if(NOT TC_BUILD_IOS)
  target_link_libraries(parallel omp)
  target_compile_options(parallel PUBLIC "-Xpreprocessor -fopenmp")
endif()
