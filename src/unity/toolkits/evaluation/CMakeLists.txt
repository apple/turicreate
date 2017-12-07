project(unity_toolkits)

make_library(unity_evaluation
  SOURCES
    metrics.cpp 
    unity_evaluation.cpp
  REQUIRES
    unity_core 
    unity_recsys
  )
