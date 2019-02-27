include(RunCMake)

set(RunCMake_TEST_OPTIONS
    "-DCMAKE_CROSSCOMPILING_EMULATOR=${PSEUDO_EMULATOR}")

run_cmake(CrosscompilingEmulatorProperty)
run_cmake(TryRun)
run_cmake(AddTest)

function(CustomCommandGenerator_run_and_build case)
  # Use a single build tree for a few tests without cleaning.
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/${case}-build)
  set(RunCMake_TEST_NO_CLEAN 1)
  set(RunCMake_TEST_OPTIONS
    "-DCMAKE_CROSSCOMPILING_EMULATOR=${PSEUDO_EMULATOR_CUSTOM_COMMAND}")
  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")
  run_cmake(${case})
  run_cmake_command(${case}-build ${CMAKE_COMMAND} --build .)
endfunction()

CustomCommandGenerator_run_and_build(AddCustomCommand)
CustomCommandGenerator_run_and_build(AddCustomTarget)
