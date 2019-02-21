message(STATUS "testname='${testname}'")

if(testname STREQUAL empty) # pass
  cmake_minimum_required()

elseif(testname STREQUAL bogus) # fail
  cmake_minimum_required(BOGUS)

elseif(testname STREQUAL no_version) # fail
  cmake_minimum_required(VERSION)

elseif(testname STREQUAL no_version_before_fatal_error) # fail
  cmake_minimum_required(VERSION FATAL_ERROR)

elseif(testname STREQUAL bad_version) # fail
  cmake_minimum_required(VERSION 2.blah.blah)

elseif(testname STREQUAL worse_version) # fail
  cmake_minimum_required(VERSION blah.blah.blah)

elseif(testname STREQUAL future_version) # fail
  math(EXPR major "${CMAKE_MAJOR_VERSION} + 1")
  cmake_minimum_required(VERSION ${major}.2.1)

elseif(testname STREQUAL unknown_arg) # fail
  cmake_minimum_required(VERSION ${CMAKE_MAJOR_VERSION}.0.0 SILLY)

else() # fail
  message(FATAL_ERROR "testname='${testname}' - error: no such test in '${CMAKE_CURRENT_LIST_FILE}'")

endif()
