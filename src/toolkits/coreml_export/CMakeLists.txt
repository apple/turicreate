project(unity_toolkits)

make_library(unity_coreml_model_export OBJECT
  SOURCES
    mldata_exporter.cpp
    xgboost_exporter.cpp
    linear_models_exporter.cpp
    neural_net_models_exporter.cpp
    coreml_export_utils.cpp
    mlmodel_wrapper.cpp
  REQUIRES
    eigen
    protobuf
    coremltools_mlmodel)

