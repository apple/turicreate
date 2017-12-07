include(RunCMake)

function(run_TargetMessages case)
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/TargetMessages-${case}-build)
  set(RunCMake_TEST_NO_CLEAN 1)
  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")
  set(RunCMake_TEST_OPTIONS "${ARGN}")
  run_cmake(TargetMessages-${case})
  run_cmake_command(TargetMessages-${case}-build ${CMAKE_COMMAND} --build .)
endfunction()

run_TargetMessages(ON)
run_TargetMessages(OFF)

run_TargetMessages(VAR-ON -DCMAKE_TARGET_MESSAGES=ON)
run_TargetMessages(VAR-OFF -DCMAKE_TARGET_MESSAGES=OFF)

run_cmake(CustomCommandDepfile-ERROR)
