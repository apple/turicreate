project(lambda)

make_library(python_callbacks
  SOURCES
    python_callbacks.cpp
  REQUIRES
    logger)

make_library(pylambda
  SOURCES
    worker_pool.cpp
    lambda_constants.cpp
    lambda_master.cpp
    pylambda_function.cpp
    graph_pylambda_master.cpp
  REQUIRES
    flexible_type cppipc sframe shmipc process python_callbacks
    EXTERNAL_VISIBILITY
)


make_library(pylambda_worker
  SOURCES
  pylambda.cpp
  pylambda_worker.cpp
  graph_pylambda.cpp
  REQUIRES
    flexible_type cppipc sframe shmipc process python_callbacks
    EXTERNAL_VISIBILITY
)

