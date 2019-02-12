project(unity_toolkits)

make_library(unity_sgd
  SOURCES
    sgd_solver_base.cpp
  REQUIRES
  unity_ml_data_2 eigen
  )
