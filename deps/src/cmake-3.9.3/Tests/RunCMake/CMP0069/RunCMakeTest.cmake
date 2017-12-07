include(RunCMake)

run_cmake(CMP0069-OLD)
run_cmake(CMP0069-NEW-cmake)
run_cmake(CMP0069-NEW-compiler)
run_cmake(CMP0069-WARN)

if(RunCMake_GENERATOR MATCHES "^Visual Studio ")
  run_cmake(CMP0069-NEW-generator)
endif()
