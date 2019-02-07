project(unity_toolkits)

add_library(unity_object_detection OBJECT
  class_registrations.cpp
  object_detector.cpp
  od_data_iterator.cpp
  od_yolo.cpp
  od_evaluation.cpp
)
add_dependencies(unity_object_detection boost)
