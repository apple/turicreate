include(RunCMake)

run_cmake(CMP0037-OLD-space)
run_cmake(CMP0037-NEW-space)
run_cmake(CMP0037-WARN-space)
run_cmake(CMP0037-NEW-colon)

if(NOT (WIN32 AND "${RunCMake_GENERATOR}" MATCHES "Make"))
  run_cmake(CMP0037-WARN-colon)
endif()

run_cmake(CMP0037-OLD-reserved)
run_cmake(CMP0037-NEW-reserved)
