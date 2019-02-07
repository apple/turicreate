project(unity_toolkits)

add_library(unity_sgd OBJECT
  sgd_solver_base.cpp
)
add_dependencies(unity_sgd boost)
