if(EXISTS "${RunCMake_TEST_BINARY_DIR}/subproject/subproject.xcodeproj")
  message(SEND_ERROR "Unexpected project file for subproject found.")
endif()
