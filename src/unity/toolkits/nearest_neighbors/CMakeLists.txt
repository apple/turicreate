project(unity_toolkits)

make_library(unity_nearest_neighbors
  SOURCES
  nearest_neighbors.cpp
  unity_nearest_neighbors.cpp 
  brute_force_neighbors.cpp
  ball_tree_neighbors.cpp
  lsh_family.cpp
  lsh_neighbors.cpp
  class_registrations.cpp
  REQUIRES
    numerics
    unity_core
    unity_ml_model
    unity_ml_data_2
  COMPILE_FLAGS_EXTRA_GCC
    -Wno-unknown-pragmas  # NOTE: used for auto-generated protobuf source files
    -Wno-unused-function  # NOTE: used for auto-generated protobuf source files
  )
