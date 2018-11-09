project(unity_toolkits)

make_library(unity_object_detection
SOURCES
    class_registrations.cpp
    object_detector.cpp
    od_data_iterator.cpp
REQUIRES
    image_io
    table_printer
    unity_core
    unity_neural_net
)
