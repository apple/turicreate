project(optimization)

make_library(optimization
  SOURCES
    utils.cpp
    lbfgs.cpp
    optimization_interface.cpp
  REQUIRES
    flexible_type
    eigen
    sframe)
