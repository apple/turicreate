project(unity_toolkits)

make_library(unity_style_transfer OBJECT
SOURCES
    style_transfer_data_iterator.cpp
REQUIRES
    unity_core
    unity_ml_model
)