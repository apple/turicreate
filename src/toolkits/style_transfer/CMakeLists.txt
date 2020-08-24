project(Turi)

make_library(unity_style_transfer OBJECT
  SOURCES
    st_model_trainer.cpp
    st_resnet16_model_trainer.cpp
    style_transfer.cpp
    style_transfer_data_iterator.cpp
    style_transfer_model_definition.cpp
    class_registrations.cpp
  REQUIRES
    unity_core
    unity_ml_model
    unity_neural_net
)
