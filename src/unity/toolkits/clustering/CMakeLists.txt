project(unity_toolkits)

make_library(unity_clustering
  SOURCES
    kmeans.cpp
    unity_kmeans.cpp
  REQUIRES
    numerics
    unity_core
    unity_ml_model
    unity_ml_data_2
  )
