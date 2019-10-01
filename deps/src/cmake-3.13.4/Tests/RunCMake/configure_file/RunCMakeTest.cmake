include(RunCMake)

run_cmake(NO-BOM)
run_cmake(UTF8-BOM)
run_cmake(UTF16LE-BOM)
run_cmake(UTF16BE-BOM)
run_cmake(UTF32LE-BOM)
run_cmake(UTF32BE-BOM)
run_cmake(UnknownArg)
run_cmake(DirInput)
run_cmake(DirOutput)
run_cmake(Relative)
run_cmake(BadArg)
run_cmake(NewLineStyle-NoArg)
run_cmake(NewLineStyle-WrongArg)
run_cmake(NewLineStyle-ValidArg)
run_cmake(NewLineStyle-COPYONLY)

if(RunCMake_GENERATOR MATCHES "Make")
  # Use a single build tree for a few tests without cleaning.
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/RerunCMake-build)
  set(RunCMake_TEST_NO_CLEAN 1)
  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")
  set(in_conf  "${RunCMake_TEST_BINARY_DIR}/ConfigureFileInput.txt.in")
  file(WRITE "${in_conf}" "1")

  message(STATUS "RerunCMake: first configuration...")
  run_cmake(RerunCMake)
  run_cmake_command(RerunCMake-nowork ${CMAKE_COMMAND} --build .)

  execute_process(COMMAND ${CMAKE_COMMAND} -E sleep 1) # handle 1s resolution
  message(STATUS "RerunCMake: touch configure_file input...")
  file(WRITE "${in_conf}" "1")
  run_cmake_command(RerunCMake-rerun ${CMAKE_COMMAND} --build .)
  run_cmake_command(RerunCMake-nowork ${CMAKE_COMMAND} --build .)

  execute_process(COMMAND ${CMAKE_COMMAND} -E sleep 1) # handle 1s resolution
  message(STATUS "RerunCMake: modify configure_file input...")
  file(WRITE "${in_conf}" "2")
  run_cmake_command(RerunCMake-rerun ${CMAKE_COMMAND} --build .)
  run_cmake_command(RerunCMake-nowork ${CMAKE_COMMAND} --build .)

  message(STATUS "RerunCMake: remove configure_file output...")
  file(REMOVE "${RunCMake_TEST_BINARY_DIR}/ConfigureFileOutput.txt")
  run_cmake_command(RerunCMake-rerun ${CMAKE_COMMAND} --build .)
  run_cmake_command(RerunCMake-nowork ${CMAKE_COMMAND} --build .)

  unset(RunCMake_TEST_BINARY_DIR)
  unset(RunCMake_TEST_NO_CLEAN)
endif()
