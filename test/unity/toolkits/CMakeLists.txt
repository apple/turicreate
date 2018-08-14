project(unity_test_toolkits)

add_subdirectory(feature_engineering)
add_subdirectory(ml_data_2)
add_subdirectory(synthetic_timings)
add_subdirectory(recsys)
add_subdirectory(sparse_similarity)
add_subdirectory(pattern_mining)

make_boost_test(kmeans_test.cxx
  REQUIRES test_dependencies
  COMPILE_FLAGS_EXTRA_GCC
    -Wno-unknown-pragmas  # NOTE: used for auto-generated protobuf source files
    -Wno-unused-function  # NOTE: used for auto-generated protobuf source files
  )

make_boost_test (nearest_neighbors.cxx
  REQUIRES test_dependencies
  COMPILE_FLAGS_EXTRA_GCC
    -Wno-unknown-pragmas  # NOTE: used for auto-generated protobuf source files
    -Wno-unused-function  # NOTE: used for auto-generated protobuf source files
  )

make_boost_test (topic_model.cxx
  REQUIRES test_dependencies
  COMPILE_FLAGS_EXTRA_GCC
    -Wno-unknown-pragmas  # NOTE: used for auto-generated protobuf source files
    -Wno-unused-function  # NOTE: used for auto-generated protobuf source files
  )

make_boost_test (test_evaluation.cxx
  REQUIRES test_dependencies
  COMPILE_FLAGS_EXTRA_GCC
    -Wno-unknown-pragmas  # NOTE: used for auto-generated protobuf source files
    -Wno-unused-function  # NOTE: used for auto-generated protobuf source files
  )
