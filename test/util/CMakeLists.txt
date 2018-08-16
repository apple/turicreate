project(util_test)

make_executable(time_fast_power
  SOURCES time_fast_power.cpp
  REQUIRES test_dependencies)

if(NOT WIN32)
make_executable(test_crash_handler
  SOURCES test_crash_handler.cpp
  REQUIRES test_dependencies)
endif()

make_boost_test(fast_power_test.cxx REQUIRES test_dependencies)

make_boost_test(cityhash_tc.cxx REQUIRES test_dependencies)
make_boost_test(2d_sparse_parallel_array.cxx REQUIRES test_dependencies)
make_boost_test(lru_test.cxx) 
