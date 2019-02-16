project(unity)

set(TC_ANNOTATIONS_PROTO_FILES
  build/format/cpp/annotate.pb.cc
  build/format/cpp/data.pb.cc
)

set(TC_ANNOTATIONS_SOURCE_FILES
  annotation_base.cpp
  image_classification.cpp
)

make_library(annotation
  SOURCES
    ${TC_ANNOTATIONS_PROTO_FILES}
    ${TC_ANNOTATIONS_SOURCE_FILES}
  REQUIRES
    protobuf
    process
    visualization
    EXTERNAL_VISIBILITY
)