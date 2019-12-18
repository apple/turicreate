project(Turi)

make_library(unity_ml_model OBJECT
  SOURCES
    python_model.cpp
  REQUIRES
    unity_core
    optimization
    boost
)
