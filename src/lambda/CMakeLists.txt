project(lambda)

add_library(python_callbacks OBJECT
  python_callbacks.cpp
)
add_dependencies(python_callbacks boost)

add_library(pylambda OBJECT
  worker_pool.cpp
  lambda_constants.cpp
  lambda_master.cpp
  pylambda_function.cpp
  graph_pylambda_master.cpp
)
add_dependencies(pylambda boost)

add_library(pylambda_worker OBJECT
  pylambda.cpp
  pylambda_worker.cpp
  graph_pylambda.cpp
)
add_dependencies(pylambda_worker boost)
