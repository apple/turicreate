cmake_minimum_required(VERSION 3.4)
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
