project(optimization)

make_library(optimization
  SOURCES
    utils.cpp
    optimization_interface.cpp
  REQUIRES
    flexible_type
    numerics
    armadillo
    sframe)
