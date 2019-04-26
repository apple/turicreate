
#Verify that option overwrites existing normal variable when the policy
#is set to OLD
cmake_policy(SET CMP0077 OLD)
set(OPT_LOCAL_VAR FALSE)
option(OPT_LOCAL_VAR "TEST_VAR" ON)
if(NOT OPT_LOCAL_VAR)
  message(FATAL_ERROR "option failed to overwrite existing normal variable")
endif()
