project(unity_test_toolkits)

make_boost_test(test_coreml_import.cxx
  REQUIRES
    unity_neural_net
  COMPILE_FLAGS_EXTRA_GCC
    -Wno-unknown-pragmas  # NOTE: used for auto-generated protobuf source files
    -Wno-unused-function  # NOTE: used for auto-generated protobuf source files
)
