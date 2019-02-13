project(unity_toolkits)

add_library(unity_activity_classification OBJECT
    activity_classifier.cpp
    sequence_iterator.cpp
    class_registrations.cpp
)
add_dependencies(unity_activity_classification boost)