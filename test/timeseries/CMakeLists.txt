project(test_unity_timeseries)
# this test really should be in unity
# in which case this should link against $UNITY_EXTENSION_LIBRARY
make_boost_test(gl_timeseries.cxx REQUIRES unity_shared)

endif()
