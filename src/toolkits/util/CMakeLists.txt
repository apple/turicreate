project(unity_toolkits)

make_library(unity_util OBJECT
  SOURCES
    precision_recall.cpp
    data_generators.cpp
    indexed_sframe_tools.cpp
    sframe_utils.cpp
    class_registrations.cpp
    random_sframe_generation.cpp
  REQUIRES
    unity_core
)
