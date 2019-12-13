project(Turi)

make_library(parallel OBJECT
  SOURCES
    pthread_tools.cpp
    thread_pool.cpp
    execute_task_in_native_thread.cpp
  REQUIRES
    platform_config
    logger
  EXTERNAL_VISIBILITY
)

set_source_files_properties(lambda_omp.hpp PROPERTIES COMPILE_FLAGS -fopenmp)
