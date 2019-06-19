project(unity_toolkits)

make_library(unity_pattern_mining OBJECT
  SOURCES
    class_registrations.cpp
    fp_growth.cpp
    fp_tree.cpp
    fp_tree_header.cpp
    fp_node.cpp
    fp_results_tree.cpp
    rule_mining.cpp
  REQUIRES
    unity_core
    unity_ml_model
    unity_feature_engineering
)
