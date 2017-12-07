project(unity_toolkits)

make_library(unity_util
  SOURCES
    precision_recall.cpp
    data_generators.cpp
    indexed_sframe_tools.cpp
    sframe_utils.cpp
  REQUIRES
  unity_core
  )
