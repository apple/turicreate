include(RunCMake)

function(run_cmake_CMP0060 CASE)
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/CMP0060-${CASE}-build)
  set(RunCMake_TEST_NO_CLEAN 1)
  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")

  run_cmake(CMP0060-${CASE})
  set(RunCMake_TEST_OUTPUT_MERGE 1)
  run_cmake_command(CMP0060-${CASE}-Build
    ${CMAKE_COMMAND} --build . --config Debug
    )
endfunction()

run_cmake_CMP0060(OLD)
run_cmake_CMP0060(WARN-OFF)
run_cmake_CMP0060(WARN-ON)
run_cmake_CMP0060(NEW)
