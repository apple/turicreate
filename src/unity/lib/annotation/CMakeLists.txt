project(unity)

set(TC_ANNOTATIONS_PROTO_FILES
  build/format/cpp/annotate.pb.cc
  build/format/cpp/data.pb.cc
  build/format/cpp/message.pb.cc
  build/format/cpp/meta.pb.cc
)

set(TC_ANNOTATIONS_CLASS_REGISTRATION_FILE
  class_registrations.cpp
)

set(TC_ANNOTATIONS_SOURCE_FILES
  annotation_base.cpp
  image_classification.cpp
)

make_library(annotation OBJECT
  SOURCES
    ${TC_ANNOTATIONS_SOURCE_FILES}
    ${TC_ANNOTATIONS_CLASS_REGISTRATION_FILE}
    ${TC_ANNOTATIONS_PROTO_FILES}
  REQUIRES
    protobuf
    process
    visualization
  EXTERNAL_VISIBILITY
)