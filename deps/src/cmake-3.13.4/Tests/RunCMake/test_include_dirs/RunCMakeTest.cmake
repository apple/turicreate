include(RunCMake)

function(run_TID)
  # Use a single build tree for a few tests without cleaning.
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/TID-build)
  set(RunCMake_TEST_NO_CLEAN 1)
  if(NOT RunCMake_GENERATOR_IS_MULTI_CONFIG)
    set(RunCMake_TEST_OPTIONS -DCMAKE_BUILD_TYPE=Debug)
  endif()
  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")
  run_cmake(TID)
  run_cmake_command(TID-build ${CMAKE_COMMAND} --build . --config Debug)
  run_cmake_command(TID-test ${CMAKE_CTEST_COMMAND} -C Debug)
endfunction()

run_TID()
