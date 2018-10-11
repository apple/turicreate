project(unity_toolkits)

make_library(unity_mps_graph
  SOURCES
  	class_registrations.cpp
    generator.mm
    graph.mm
  REQUIRES
    unity_core
 )
