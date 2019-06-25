project(unity_toolkits)

make_library(unity_nearest_neighbors OBJECT
  SOURCES
    nearest_neighbors.cpp
    unity_nearest_neighbors.cpp
    brute_force_neighbors.cpp
    ball_tree_neighbors.cpp
    lsh_family.cpp
    lsh_neighbors.cpp
    class_registrations.cpp
  REQUIRES
    eigen
    unity_core
    unity_ml_model
    unity_ml_data_2
)
