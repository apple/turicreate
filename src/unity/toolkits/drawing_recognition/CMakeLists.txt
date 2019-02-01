project(unity_toolkits)

make_library(unity_drawing_recognition
SOURCES
    data_preparation.cpp
    class_registrations.cpp
REQUIRES
    unity_core
    unity_ml_model
)
