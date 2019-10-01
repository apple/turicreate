include(RunCMake)

if(RunCMake_GENERATOR STREQUAL Xcode)
  if(XCODE_BELOW_6_1)
    run_cmake(XcodeTooOld)
  endif()
else()
  run_cmake(NotSupported)
endif()
