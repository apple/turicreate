make_executable(time_itemcf
   SOURCES time_itemcf.cpp
   REQUIRES test_dependencies
   COMPILE_FLAGS_EXTRA_GCC
     -Wno-unknown-pragmas  # NOTE: used for auto-generated protobuf source files
     -Wno-unused-function  # NOTE: used for auto-generated protobuf source files
   )

make_executable(time_recsys_data_prep
  SOURCES time_recsys_data_prep
  REQUIRES test_dependencies
  COMPILE_FLAGS_EXTRA_GCC
    -Wno-unknown-pragmas  # NOTE: used for auto-generated protobuf source files
    -Wno-unused-function  # NOTE: used for auto-generated protobuf source files
  )

make_executable(time_recsys_sgd
  SOURCES time_recsys_sgd
  REQUIRES test_dependencies
  COMPILE_FLAGS_EXTRA_GCC
    -Wno-unknown-pragmas  # NOTE: used for auto-generated protobuf source files
    -Wno-unused-function  # NOTE: used for auto-generated protobuf source files
  )

make_executable(time_nearest_neighbors
  SOURCES time_nearest_neighbors
  REQUIRES test_dependencies
  COMPILE_FLAGS_EXTRA_GCC
    -Wno-unknown-pragmas  # NOTE: used for auto-generated protobuf source files
    -Wno-unused-function  # NOTE: used for auto-generated protobuf source files
  )
