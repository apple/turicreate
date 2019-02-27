include(RunCMake)

function(run_TargetWithCommand)
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/TargetWithCommand-build)
  run_cmake(TargetWithCommand)
  set(RunCMake_TEST_NO_CLEAN 1)
  run_cmake_command(TargetWithCommand-build ${CMAKE_COMMAND} --build . --config Debug)
endfunction()
run_TargetWithCommand()

# Use a single build tree for a few tests without cleaning.
set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/CommandWithOutput-build)
set(RunCMake_TEST_NO_CLEAN 1)
file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")
set(RunCMake-check-file CommandWithOutput-check.cmake)

set(srcName "test.cs")
set(srcFileName "${CMAKE_CURRENT_LIST_DIR}/${srcName}.in")
set(inputFileName "${RunCMake_TEST_BINARY_DIR}/${srcName}.in")
set(generatedFileName "${RunCMake_TEST_BINARY_DIR}/${srcName}")
set(commandComment "Generating ${srcName}")

# copy the input file to build dir to avoid changing files in cmake
# source tree.
file(COPY "${srcFileName}" DESTINATION "${RunCMake_TEST_BINARY_DIR}")

set(RunCMake_TEST_OPTIONS ${RunCMake_TEST_OPTIONS}
  "-DinputFileName=${inputFileName}"
  "-DgeneratedFileName=${generatedFileName}"
  "-DcommandComment=${commandComment}")

set(checkLevel 0)
run_cmake(CommandWithOutput)
set(checkLevel 1)
run_cmake_command(CommandWithOutput-build1 ${CMAKE_COMMAND} --build . --config Debug)
set(checkLevel 2)
run_cmake_command(CommandWithOutput-build2 ${CMAKE_COMMAND} --build . --config Debug)
# change file content to trigger custom command with next build
file(APPEND ${inputFileName} "\n")
set(checkLevel 3)
run_cmake_command(CommandWithOutput-build3 ${CMAKE_COMMAND} --build . --config Debug)
