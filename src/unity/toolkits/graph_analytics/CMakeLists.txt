project(unity_toolkits)

make_library(unity_graph_analytics OBJECT
  SOURCES
    kcore_sgraph.cpp
    connected_component_sgraph.cpp
    graph_coloring_sgraph.cpp
    triangle_counting_sgraph.cpp
    sssp_sgraph.cpp
    pagerank_sgraph.cpp
    degree_count.cpp
    label_propagation.cpp
  REQUIRES
    unity_core
)
