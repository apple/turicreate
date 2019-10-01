include(RunCMake)


function(run_SymlinkImplicit)
  # Use a single build tree for a few tests without cleaning.
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/SymlinkImplicit-build)
  set(RunCMake_TEST_NO_CLEAN 1)
  if(NOT RunCMake_GENERATOR_IS_MULTI_CONFIG)
    set(RunCMake_TEST_OPTIONS -DCMAKE_BUILD_TYPE=Debug)
  endif()
  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")
  run_cmake(SymlinkImplicit)
  run_cmake_command(SymlinkImplicit-build ${CMAKE_COMMAND} --build . --config Debug)
  run_cmake_command(SymlinkImplicitCheck
    ${CMAKE_COMMAND} -Ddir=${RunCMake_TEST_BINARY_DIR} -P ${RunCMake_SOURCE_DIR}/SymlinkImplicitCheck.cmake)
endfunction()
run_SymlinkImplicit()
