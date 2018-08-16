make_executable(time_itemcf
   SOURCES time_itemcf.cpp
   REQUIRES unity_core unity_recsys
   COMPILE_FLAGS_EXTRA_GCC
     -Wno-unknown-pragmas  # NOTE: used for auto-generated protobuf source files
     -Wno-unused-function  # NOTE: used for auto-generated protobuf source files
   )

make_executable(time_recsys_data_prep
  SOURCES time_recsys_data_prep
  REQUIRES unity_recsys  unity_core
  COMPILE_FLAGS_EXTRA_GCC
    -Wno-unknown-pragmas  # NOTE: used for auto-generated protobuf source files
    -Wno-unused-function  # NOTE: used for auto-generated protobuf source files
  )

make_executable(time_recsys_sgd
  SOURCES time_recsys_sgd
  REQUIRES unity_recsys  unity_core
  COMPILE_FLAGS_EXTRA_GCC
    -Wno-unknown-pragmas  # NOTE: used for auto-generated protobuf source files
    -Wno-unused-function  # NOTE: used for auto-generated protobuf source files
  )

make_executable(time_nearest_neighbors
  SOURCES time_nearest_neighbors
  REQUIRES unity_util unity_ml_data_2 unity_core unity_nearest_neighbors
  COMPILE_FLAGS_EXTRA_GCC
    -Wno-unknown-pragmas  # NOTE: used for auto-generated protobuf source files
    -Wno-unused-function  # NOTE: used for auto-generated protobuf source files
  )
