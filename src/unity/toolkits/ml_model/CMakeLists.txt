project(unity_toolkits)

make_library(unity_ml_model
  SOURCES
  python_model.cpp
  REQUIRES
    unity_core
    optimization
  )
