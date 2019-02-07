project(parallel)

add_library(parallel OBJECT
  pthread_tools.cpp
  thread_pool.cpp
  execute_task_in_native_thread.cpp
)
add_dependencies(parallel boost)