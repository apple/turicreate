project(util_test)

make_executable(time_fast_power
  SOURCES time_fast_power.cpp
  REQUIRES unity_shared)

if(NOT WIN32)
  # TODO - seems we can delete crash_handler? Nothing is using it.
  #make_executable(test_crash_handler
  #  SOURCES test_crash_handler.cpp
  #  REQUIRES unity_shared)
endif()

make_boost_test(fast_power_test.cxx REQUIRES unity_shared)

make_boost_test(cityhash_tc.cxx REQUIRES unity_shared)
make_boost_test(2d_sparse_parallel_array.cxx REQUIRES unity_shared)
make_boost_test(lru_test.cxx) 
