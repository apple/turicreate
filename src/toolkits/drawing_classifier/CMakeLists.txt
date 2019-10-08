project(unity_toolkits)

make_library(unity_drawing_classifier OBJECT
SOURCES
    drawing_classifier.cpp
    data_preparation.cpp
    class_registrations.cpp
REQUIRES
    unity_core
    unity_ml_model
)
