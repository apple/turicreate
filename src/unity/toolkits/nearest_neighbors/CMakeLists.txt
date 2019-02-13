project(unity_toolkits)

add_library(unity_nearest_neighbors OBJECT
  nearest_neighbors.cpp
  unity_nearest_neighbors.cpp 
  brute_force_neighbors.cpp
  ball_tree_neighbors.cpp
  lsh_family.cpp
  lsh_neighbors.cpp
  class_registrations.cpp
)
add_dependencies(unity_nearest_neighbors boost)
