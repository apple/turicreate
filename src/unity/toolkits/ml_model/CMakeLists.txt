project(unity_toolkits)

make_library(unity_ml_model
  SOURCES
  ml_model.cpp
  sdk_model.cpp
  python_model.cpp
  REQUIRES
    unity_core
    unity_options
    optimization
  )
