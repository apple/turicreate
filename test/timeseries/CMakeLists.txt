project(test_unity_timeseries)
if (WIN32)
make_boost_test(gl_timeseries.cxx REQUIRES unity_core_shared timeseries)
else()
make_boost_test(gl_timeseries.cxx REQUIRES unity_shared timeseries)

if(APPLE)
  SET_TARGET_PROPERTIES(gl_timeseries.cxxtest PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()

endif()
