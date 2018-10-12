project(unity_toolkits)

if(HAS_MPS)
  set(additional_unity_object_detection_requirements tcmps)
else()
  set(additional_unity_object_detection_requirements "")
endif()

make_library(unity_object_detection
SOURCES
    object_detector.cpp
    class_registrations.cpp
REQUIRES
    unity_core
    ${additional_unity_object_detection_requirements}
)
