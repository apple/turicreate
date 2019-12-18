project(Turi)

make_library(unity_drawing_classifier OBJECT
SOURCES
    class_registrations.cpp
    data_preparation.cpp
    dc_data_iterator.cpp
    drawing_classifier.cpp
REQUIRES
    unity_core
    unity_ml_model
)
