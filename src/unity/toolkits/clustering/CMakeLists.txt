project(unity_toolkits)

make_library(unity_clustering
  SOURCES
    kmeans.cpp
    class_registrations.cpp
    unity_kmeans.cpp
  REQUIRES
    numerics
    unity_core
    unity_ml_model
    unity_ml_data_2
  COMPILE_FLAGS_EXTRA_GCC
    -Wno-unknown-pragmas  # NOTE: used for auto-generated protobuf source files
    -Wno-unused-function  # NOTE: used for auto-generated protobuf source files
  )
