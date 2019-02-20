project(optimization)

make_library(optimization OBJECT
  SOURCES
    utils.cpp
    lbfgs.cpp
    optimization_interface.cpp
  REQUIRES
    flexible_type		
    eigen		
    sframe
)
