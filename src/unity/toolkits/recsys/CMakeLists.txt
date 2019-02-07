project(unity_toolkits)

add_library(unity_recsys OBJECT
  class_registrations.cpp
  recsys_model_base.cpp
  train_test_split.cpp
  user_item_lists.cpp
  user_item_graph.cpp
  models/popularity.cpp
  models/itemcf.cpp
  models/factorization_models.cpp
)
add_dependencies(unity_recsys boost)