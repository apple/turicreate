project(unity_toolkits)

add_library(unity_clustering OBJECT
  kmeans.cpp
  class_registrations.cpp
  unity_kmeans.cpp
)
add_dependencies(unity_clustering boost)
