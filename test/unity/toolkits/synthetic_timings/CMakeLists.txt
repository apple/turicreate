make_executable(time_itemcf
   SOURCES time_itemcf.cpp
   REQUIRES unity_shared_for_testing
)

make_executable(time_recsys_data_prep
  SOURCES time_recsys_data_prep
  REQUIRES unity_shared_for_testing
)

make_executable(time_recsys_sgd
  SOURCES time_recsys_sgd
  REQUIRES unity_shared_for_testing
)

make_executable(time_nearest_neighbors
  SOURCES time_nearest_neighbors
  REQUIRES unity_shared_for_testing
)
