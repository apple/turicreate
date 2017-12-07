enable_language(C)

find_library(XCTEST_LIBRARY XCTest)
if(NOT XCTEST_LIBRARY)
  message(FATAL_ERROR "XCTest Framework not found.")
endif()
