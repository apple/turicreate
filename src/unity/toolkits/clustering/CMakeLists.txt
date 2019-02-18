project(unity_toolkits)

make_library(unity_clustering OBJECT
  SOURCES
    kmeans.cpp
    class_registrations.cpp
    unity_kmeans.cpp
  REQUIRES
    unity_core
    unity_ml_model
    unity_ml_data_2
)
