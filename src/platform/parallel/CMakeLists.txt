project(parallel)

make_library(parallel 
  SOURCES
    pthread_tools.cpp
    thread_pool.cpp
    execute_task_in_native_thread.cpp
  REQUIRES
    logger
    EXTERNAL_VISIBILITY
)
