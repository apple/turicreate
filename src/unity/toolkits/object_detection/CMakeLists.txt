project(unity_toolkits)

make_library(unity_object_detection
SOURCES
    object_detector.cpp
    class_registrations.cpp
REQUIRES
    unity_core
    unity_neural_net
)
