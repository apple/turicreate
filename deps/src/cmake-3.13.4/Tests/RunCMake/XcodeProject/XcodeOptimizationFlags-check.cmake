foreach(level 1 2 3 s fast)
  file(STRINGS ${RunCMake_TEST_BINARY_DIR}/XcodeOptimizationFlags.xcodeproj/project.pbxproj actual-${level}
       REGEX "GCC_OPTIMIZATION_LEVEL = ${level};" LIMIT_COUNT 1)
  if(NOT actual-${level})
    message(SEND_ERROR "Optimization level '${level}' not found in Xcode project.")
  endif()
endforeach()
