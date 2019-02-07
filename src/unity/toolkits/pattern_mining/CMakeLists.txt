project(unity_toolkits)

add_library(unity_pattern_mining OBJECT
  class_registrations.cpp
  fp_growth.cpp
  fp_tree.cpp
  fp_tree_header.cpp
  fp_node.cpp
  fp_results_tree.cpp
  rule_mining.cpp
)
add_dependencies(unity_pattern_mining boost)
