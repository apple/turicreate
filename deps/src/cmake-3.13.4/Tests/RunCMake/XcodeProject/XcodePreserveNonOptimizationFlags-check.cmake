file(STRINGS ${RunCMake_TEST_BINARY_DIR}/XcodePreserveNonOptimizationFlags.xcodeproj/project.pbxproj actual
     REGEX "OTHER_CPLUSPLUSFLAGS = [^;]*;")
foreach(expect "-DA" "-DB +-DC" "-DD")
  if(NOT "${actual}" MATCHES "${expect}")
    message(SEND_ERROR "The actual project contains the lines:\n ${actual}\n"
      "which do not match expected regex:\n ${expect}\n")
  endif()
endforeach()
