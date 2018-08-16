project(util_test)

make_executable(time_fast_power
  SOURCES time_fast_power.cpp
  REQUIRES util logger random)

if(NOT WIN32)
make_executable(test_crash_handler
  SOURCES test_crash_handler.cpp
  REQUIRES util crash_handler)
endif()

make_boost_test(fast_power_test.cxx REQUIRES util logger random)

make_boost_test(cityhash_tc.cxx REQUIRES util logger random flexible_type)
make_boost_test(2d_sparse_parallel_array.cxx REQUIRES util logger random)
make_boost_test(lru_test.cxx) 
