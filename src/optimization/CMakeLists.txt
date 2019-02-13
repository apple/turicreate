project(optimization)

add_library(optimization OBJECT
  utils.cpp
  optimization_interface.cpp
)
add_dependencies(optimization boost)
