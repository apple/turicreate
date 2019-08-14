project(unity)

set(FORMAT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/format")
set(BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/format/cpp")

set(ANNOTATE_PROTO "${CMAKE_CURRENT_SOURCE_DIR}/format/annotate.proto")
set(DATA_PROTO "${CMAKE_CURRENT_SOURCE_DIR}/format/data.proto")
set(MESSAGE_PROTO "${CMAKE_CURRENT_SOURCE_DIR}/format/message.proto")
set(META_PROTO "${CMAKE_CURRENT_SOURCE_DIR}/format/meta.proto")
set(PROGRESS_PROTO "${CMAKE_CURRENT_SOURCE_DIR}/format/progress.proto")
set(SIMILARITY_PROTO "${CMAKE_CURRENT_SOURCE_DIR}/format/similarity.proto")

set(TC_ANNOTATIONS_PROTO_FILES
  build/format/cpp/annotate.pb.cc
  build/format/cpp/data.pb.cc
  build/format/cpp/message.pb.cc
  build/format/cpp/meta.pb.cc
  build/format/cpp/progress.pb.cc
  build/format/cpp/similarity.pb.cc
)

set(TC_ANNOTATIONS_PROTO_HEADERS
  build/format/cpp/annotate.pb.h
  build/format/cpp/data.pb.h
  build/format/cpp/message.pb.h
  build/format/cpp/meta.pb.h
  build/format/cpp/progress.pb.h
  build/format/cpp/similarity.pb.h
)

add_custom_target(annotationProtobuf)
add_custom_command(TARGET annotationProtobuf
  PRE_BUILD
  COMMAND  protoc --proto_path=${FORMAT_DIR}
                  --cpp_out=${BUILD_DIR} ${ANNOTATE_PROTO}
                                          ${DATA_PROTO}
                                          ${MESSAGE_PROTO}
                                          ${META_PROTO}
                                          ${PROGRESS_PROTO}
                                          ${SIMILARITY_PROTO}
)

set(TC_ANNOTATIONS_SOURCE_FILES
  annotation_base.cpp
  class_registrations.cpp
  image_classification.cpp
  object_detection.cpp
  utils.cpp
)

make_library(annotation OBJECT
  SOURCES
    ${TC_ANNOTATIONS_SOURCE_FILES}
    ${TC_ANNOTATIONS_PROTO_FILES}
  REQUIRES
    protobuf
    process
    visualization
    image_deep_feature_extractor
  EXTERNAL_VISIBILITY
)