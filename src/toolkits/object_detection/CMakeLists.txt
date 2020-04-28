project(Turi)

make_library(unity_object_detection OBJECT
  SOURCES
    class_registrations.cpp
    object_detector.cpp
    od_darknet_yolo_model_trainer.cpp
    od_data_iterator.cpp
    od_evaluation.cpp
    od_model_trainer.cpp
    od_serialization.cpp
    od_yolo.cpp
  REQUIRES
    image_io
    random
    table_printer
    unity_core
    unity_neural_net
)

subdirs(one_shot_object_detection)
