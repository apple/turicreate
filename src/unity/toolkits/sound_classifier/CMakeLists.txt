project(unity_toolkits)

make_library(unity_sound_classifier OBJECT
SOURCES
    sound_classifier.cpp
REQUIRES
    unity_core
    unity_ml_model
)
