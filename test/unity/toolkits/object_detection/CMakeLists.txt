project(unity_test_toolkits)

make_boost_test(test_object_detector.cxx REQUIRES unity_toolkits)

make_boost_test(test_od_data_iterator.cxx REQUIRES unity_toolkits)

make_boost_test(test_od_yolo.cxx
  REQUIRES
    unity_toolkits
  COMPILE_FLAGS_EXTRA_GCC
    -Wno-unknown-pragmas  # NOTE: used for auto-generated protobuf source files
    -Wno-unused-function  # NOTE: used for auto-generated protobuf source files
)

make_boost_test(test_od_evaluation.cxx REQUIRES unity_toolkits)
