file(READ ${stamp} content)
if(NOT content STREQUAL 2)
  set(RunCMake_TEST_FAILED "Expected stamp '2' but got: '${content}'")
endif()

file(READ ${output} content)
if(NOT content STREQUAL 2)
  set(RunCMake_TEST_FAILED "Expected output '2' but got: '${content}'")
endif()
