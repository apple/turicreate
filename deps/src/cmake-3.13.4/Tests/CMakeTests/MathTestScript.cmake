message(STATUS "testname='${testname}'")

if(testname STREQUAL empty) # fail
  math()

elseif(testname STREQUAL bogus) # fail
  math(BOGUS)

elseif(testname STREQUAL not_enough_args) # fail
  math(EXPR x)

elseif(testname STREQUAL cannot_parse) # fail
  math(EXPR x "1 + 2 +")

else() # fail
  message(FATAL_ERROR "testname='${testname}' - error: no such test in '${CMAKE_CURRENT_LIST_FILE}'")

endif()
