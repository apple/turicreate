project(unity_toolkits)

add_library(unity_util OBJECT
  precision_recall.cpp
  data_generators.cpp
  indexed_sframe_tools.cpp
  sframe_utils.cpp
  class_registrations.cpp
  random_sframe_generation.cpp
)
add_dependencies(unity_util boost)