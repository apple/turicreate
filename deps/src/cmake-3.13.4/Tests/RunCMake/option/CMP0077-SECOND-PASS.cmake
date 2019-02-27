
#Verify that when both a cache and local version of a value exist that CMake
#doesn't produce a CMP0077 warning and that we get the expected values.
option(OPT_LOCAL_VAR "TEST_VAR" ON)
set(OPT_LOCAL_VAR FALSE)
option(OPT_LOCAL_VAR "TEST_VAR" ON)
if(OPT_LOCAL_VAR)
  message(FATAL_ERROR "option improperly set a cache variable that already exists")
endif()

get_property(_exists_in_cache CACHE OPT_LOCAL_VAR PROPERTY VALUE SET)
if(NOT _exists_in_cache)
  message(FATAL_ERROR "value should exist in cache")
endif()
