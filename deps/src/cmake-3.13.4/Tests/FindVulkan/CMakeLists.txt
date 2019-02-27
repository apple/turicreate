add_test(NAME FindVulkan.Test COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindVulkan/Test"
  "${CMake_BINARY_DIR}/Tests/FindVulkan/Test"
  ${build_generator_args}
  --build-project TestFindVulkan
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )
