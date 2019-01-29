project(unity_test_toolkits)

make_boost_test(test_neural_nets_model_exporter.cxx
  REQUIRES
    unity_toolkits
  COMPILE_FLAGS_EXTRA_GCC
    -Wno-unknown-pragmas  # NOTE: used for auto-generated protobuf source files
    -Wno-unused-function  # NOTE: used for auto-generated protobuf source files
)
