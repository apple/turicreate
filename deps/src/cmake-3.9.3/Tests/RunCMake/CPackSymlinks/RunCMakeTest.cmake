include(RunCMake)

function(run_cpack_symlink_test)
  set(RunCMake_TEST_NO_CLEAN TRUE)
  set(RunCMake_TEST_BINARY_DIR "${RunCMake_BINARY_DIR}/SrcSymlinks-build")
  set(RunCMake_TEST_SOURCE_DIR "${RunCMake_BINARY_DIR}/SrcSymlinks")
  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(REMOVE_RECURSE "${RunCMake_TEST_SOURCE_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_SOURCE_DIR}")
  run_cmake_command(SrcSymlinksTar
    ${CMAKE_COMMAND} -E chdir ${RunCMake_TEST_SOURCE_DIR}
    ${CMAKE_COMMAND} -E tar xvf ${RunCMake_SOURCE_DIR}/testcpacksym.tar
    )
  run_cmake(SrcSymlinksCMake)
  run_cmake_command(SrcSymlinksCPack
    ${CMAKE_CPACK_COMMAND} --config CPackSourceConfig.cmake
    )
endfunction()

run_cpack_symlink_test()
