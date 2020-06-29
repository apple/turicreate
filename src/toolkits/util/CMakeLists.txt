project(Turi)

if(APPLE)
  set(additional_unity_util_sources image_utils_Apple.mm)
else()
  set(additional_unity_util_sources image_utils_Portable.cpp)
endif()

make_library(unity_util OBJECT
  SOURCES
    class_registrations.cpp
    data_generators.cpp
    float_array_serialization.cpp
    indexed_sframe_tools.cpp
    precision_recall.cpp
    random_sframe_generation.cpp
    sframe_utils.cpp
    ${additional_unity_util_sources}
  REQUIRES
    unity_core
    unity_neural_net
)
