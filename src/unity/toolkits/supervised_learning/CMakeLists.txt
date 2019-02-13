project(unity_toolkits)

add_library(supervised_learning OBJECT
  supervised_learning.cpp
  linear_regression.cpp
  linear_regression_opt_interface.cpp
  logistic_regression.cpp
  logistic_regression_opt_interface.cpp
  linear_svm.cpp
  linear_svm_opt_interface.cpp
  xgboost.cpp
  xgboost_iterator.cpp
  boosted_trees.cpp
  random_forest.cpp
  decision_tree.cpp
  xgboost_error.cpp
  automatic_model_creation.cpp
  classifier_evaluations.cpp
  class_registrations.cpp
)
add_dependencies(supervised_learning boost)
target_compile_definitions(supervised_learning PUBLIC "-DXGBOOST_CUSTOMIZE_MSG_")
