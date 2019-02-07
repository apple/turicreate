project(unity_toolkits)

add_library(unity_factorization OBJECT
  factory_instantiations/basic_sgd_squared_error.cpp
  factory_instantiations/basic_sgd_logistic.cpp
  factory_instantiations/ranking_squared_error.cpp
  factory_instantiations/ranking_logistic.cpp
  model_factory.cpp
  factorization_model_serialization.cpp
  factorization_model.cpp
  factorization_model_options.cpp
  loss_model_profiles.cpp
  factorization_model_training.cpp
)
add_dependencies(unity_factorization boost)