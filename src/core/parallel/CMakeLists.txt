project(parallel)

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
