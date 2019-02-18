include(RunCMake)


function(run_lwyu lang)
  # Use a single build tree for tests without cleaning.
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/${lang}-build)
  set(RunCMake_TEST_NO_CLEAN 1)
  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")
  run_cmake(${lang})

  set(RunCMake_TEST_OUTPUT_MERGE 1)
  run_cmake_command(${lang}-Build ${CMAKE_COMMAND} --build .)
endfunction()

run_lwyu(CXX)
run_lwyu(C)
if (NOT RunCMake_GENERATOR STREQUAL "Watcom WMake")
  run_lwyu(C-launch)
  run_lwyu(CXX-launch)
endif()
