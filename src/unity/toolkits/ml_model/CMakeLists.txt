project(unity_toolkits)

make_library(unity_ml_model OBJECT
  SOURCES
    python_model.cpp
  REQUIRES
    unity_core
    optimization
)
