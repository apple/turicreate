project(unity_toolkits)

make_library(unity_activity_classification
SOURCES
    activity_classifier.cpp
    sequence_iterator.cpp
    class_registrations.cpp
REQUIRES
    unity_core
    unity_ml_model
)
