add_test(NAME FindJPEG.Test COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindJPEG/Test"
  "${CMake_BINARY_DIR}/Tests/FindJPEG/Test"
  ${build_generator_args}
  --build-project TestFindJPEG
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )
