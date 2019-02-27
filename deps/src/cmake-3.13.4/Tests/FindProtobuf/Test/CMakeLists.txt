cmake_minimum_required(VERSION 3.8)
project(TestFindProtobuf CXX)
include(CTest)

find_package(Protobuf REQUIRED)

add_executable(test_tgt main.cxx)
target_link_libraries(test_tgt protobuf::libprotobuf)
add_test(NAME test_tgt COMMAND test_tgt)

add_executable(test_var main.cxx)
target_include_directories(test_var PRIVATE ${Protobuf_INCLUDE_DIRS})
target_link_libraries(test_var PRIVATE ${Protobuf_LIBRARIES})
add_test(NAME test_var COMMAND test_var)

add_executable(test_tgt_lite main.cxx)
target_link_libraries(test_tgt_lite protobuf::libprotobuf-lite)
add_test(NAME test_tgt_lite COMMAND test_tgt_lite)

add_executable(test_var_lite main.cxx)
target_include_directories(test_var_lite PRIVATE ${Protobuf_INCLUDE_DIRS})
target_link_libraries(test_var_lite PRIVATE ${Protobuf_LITE_LIBRARIES})
add_test(NAME test_var_lite COMMAND test_var_lite)

add_executable(test_tgt_protoc main-protoc.cxx)
target_link_libraries(test_tgt_protoc protobuf::libprotoc)
add_test(NAME test_tgt_protoc COMMAND test_tgt_protoc)

add_executable(test_var_protoc main-protoc.cxx)
target_include_directories(test_var_protoc PRIVATE ${Protobuf_INCLUDE_DIRS})
target_link_libraries(test_var_protoc PRIVATE ${Protobuf_PROTOC_LIBRARIES})
add_test(NAME test_var_protoc COMMAND test_var_protoc)

add_test(NAME test_tgt_protoc_version COMMAND protobuf::protoc --version)

set(Protobuf_IMPORT_DIRS ${Protobuf_INCLUDE_DIRS})
PROTOBUF_GENERATE_CPP(PROTO_SRC PROTO_HEADER msgs/example.proto)
PROTOBUF_GENERATE_CPP(DESC_PROTO_SRC DESC_PROTO_HEADER DESCRIPTORS DESC_PROTO_DESC msgs/example_desc.proto)
add_library(msgs ${PROTO_SRC} ${PROTO_HEADER})

add_executable(test_generate main-generate.cxx ${PROTO_SRC})
target_include_directories(test_generate PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(test_generate msgs ${Protobuf_LIBRARIES})
add_test(NAME test_generate COMMAND test_generate)

add_executable(test_desc main-desc.cxx ${DESC_PROTO_SRC})
target_compile_features(test_desc PRIVATE cxx_std_11)
target_include_directories(test_desc PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(test_desc msgs ${Protobuf_LIBRARIES})
add_test(NAME test_desc COMMAND test_desc ${DESC_PROTO_DESC})
