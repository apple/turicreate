project(unity_toolkits)

make_library(unity_object_detection OBJECT
  SOURCES
    class_registrations.cpp
    object_detector.cpp
    od_data_iterator.cpp
    od_yolo.cpp
    od_evaluation.cpp
    od_serialization.cpp
  REQUIRES
    image_io
    random
    table_printer
    unity_core
    unity_neural_net
)

subdirs(one_shot_object_detection)
