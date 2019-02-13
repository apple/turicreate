project(unity_toolkits)

add_library(unity_evaluation OBJECT
  metrics.cpp 
  unity_evaluation.cpp
)
add_dependencies(unity_evaluation boost)
