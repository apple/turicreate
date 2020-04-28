project(Turi)

make_library(unity_util OBJECT
  SOURCES
    class_registrations.cpp
    data_generators.cpp
    float_array_serialization.cpp
    indexed_sframe_tools.cpp
    precision_recall.cpp
    random_sframe_generation.cpp
    sframe_utils.cpp
  REQUIRES
    unity_core
    unity_neural_net
)
