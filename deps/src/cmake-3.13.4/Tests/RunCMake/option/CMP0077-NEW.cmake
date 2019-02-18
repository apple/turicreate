
#Verify that option DOESN'T overwrite existing normal variable when the policy
#is set to NEW
cmake_policy(SET CMP0077 NEW)
set(OPT_LOCAL_VAR FALSE)
option(OPT_LOCAL_VAR "TEST_VAR" ON)
if(OPT_LOCAL_VAR)
  message(FATAL_ERROR "option failed to overwrite existing normal variable")
endif()

get_property(_exists_in_cache CACHE OPT_LOCAL_VAR PROPERTY VALUE SET)
if(_exists_in_cache)
  message(FATAL_ERROR "value should not exist in cache as it was already a local variable")
endif()
