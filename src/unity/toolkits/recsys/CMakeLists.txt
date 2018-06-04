project(unity_toolkits)

make_library(unity_recsys
  SOURCES
    class_registrations.cpp
    unity_recsys.cpp
    recsys_model_base.cpp
    train_test_split.cpp
    user_item_lists.cpp
    user_item_graph.cpp
    models/popularity.cpp
    models/itemcf.cpp
    models/factorization_models.cpp
  REQUIRES
    numerics
    unity_sparse_similarity
    unity_ml_model
    unity_ml_data_2
    unity_util
    unity_options
    unity_nearest_neighbors
    # unity_evaluation
    unity_coreml_model_export
    unity_factorization
    unity_core)
