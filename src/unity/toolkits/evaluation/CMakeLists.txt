project(unity_toolkits)

make_library(unity_evaluation OBJECT
  SOURCES
    metrics.cpp 
    unity_evaluation.cpp
  REQUIRES
    ml_data
    unity_core
    unity_recsys
)
