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

# TODO - the pragma warning supression doesn't seem to work in gcc 5.5.0.
# This is needed to suppress warnings from protobuf headers.
add_compile_options(-Wno-unused-function)