project(unity_toolkits)

make_library(unity_one_shot_object_detection OBJECT
  SOURCES
    class_registrations.cpp
    util/parameter_sampler.cpp
    util/quadrilateral_geometry.cpp
    util/superposition.cpp
    one_shot_object_detector.cpp
  REQUIRES
    image_io
    random
    table_printer
    unity_core
)
