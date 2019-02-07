project(network)

add_library(network OBJECT
  net_util.cpp
)
add_dependencies(network boost)