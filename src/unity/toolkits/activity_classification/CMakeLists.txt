project(unity_toolkits)

make_library(unity_activity_classification OBJECT
    SOURCES
        ac_data_iterator.cpp
        activity_classifier.cpp
        class_registrations.cpp
    REQUIRES
        unity_core
        unity_ml_model
)
