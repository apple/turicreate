include(RunCMake)

set(RunCMake_TEST_OPTIONS "-DPSEUDO_CPPCHECK=${PSEUDO_CPPCHECK}")

function(run_cppcheck lang)
  # Use a single build tree for tests without cleaning.
  set(RunCMake_TEST_BINARY_DIR "${RunCMake_BINARY_DIR}/${lang}-build")
  set(RunCMake_TEST_NO_CLEAN 1)
  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")
  run_cmake(${lang})
  set(RunCMake_TEST_OUTPUT_MERGE 1)
  run_cmake_command(${lang}-Build ${CMAKE_COMMAND} --build .)
endfunction()

run_cppcheck(C)
run_cppcheck(CXX)
run_cppcheck(C-bad)

if(NOT RunCMake_GENERATOR STREQUAL "Watcom WMake")
  run_cppcheck(C-launch)
  run_cppcheck(CXX-launch)
endif()
