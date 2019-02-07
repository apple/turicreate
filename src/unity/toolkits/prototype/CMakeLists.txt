project(unity_toolkits)

add_library(unity_prototype OBJECT
  sparse_nn.cpp
  class_registrations.cpp
)
add_dependencies(unity_prototype boost)
