add_test(NAME FindLibXml2.Test COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindLibXml2/Test"
  "${CMake_BINARY_DIR}/Tests/FindLibXml2/Test"
  ${build_generator_args}
  --build-project TestFindLibXml2
  --build-options ${build_options}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )
