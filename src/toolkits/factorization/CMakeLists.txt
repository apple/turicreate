project(unity_toolkits)

make_library(unity_factorization OBJECT
  SOURCES
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
  REQUIRES
    unity_ml_data_2 unity_sgd eigen
)
add_dependencies(unity_factorization boost)
