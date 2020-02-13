project(Turi)

make_library(
  parallel
  SOURCES
  pthread_tools.cpp
  thread_pool.cpp
  execute_task_in_native_thread.cpp
  lambda_omp.cpp
  EXTERNAL_VISIBILITY)

if(NOT TC_BUILD_IOS)
  target_link_libraries(parallel PRIVATE omp)
  target_compile_options(parallel PRIVATE "-Xpreprocessor -fopenmp")
endif()
