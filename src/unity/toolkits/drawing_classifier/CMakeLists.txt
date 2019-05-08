project(unity_toolkits)

make_library(unity_drawing_classifier OBJECT
SOURCES
	drawing_classifier.mm
    data_preparation.cpp
    mps_drawing_classifier.mm
    class_registrations.cpp
REQUIRES
    unity_core
    unity_ml_model
)
