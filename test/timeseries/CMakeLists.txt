project(test_unity_timeseries)
# this test really should be in unity
make_boost_test(gl_timeseries.cxx REQUIRES unity_shared_for_testing)

endif()
