if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMake_TEST_FindOpenGL_VND)
  set(_vnd_testing TRUE)
else()
  set(_vnd_testing FALSE)
endif()

add_test(NAME FindOpenGL.Test COMMAND
  ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION>
  --build-and-test
  "${CMake_SOURCE_DIR}/Tests/FindOpenGL/Test"
  "${CMake_BINARY_DIR}/Tests/FindOpenGL/Test"
  ${build_generator_args}
  --build-project TestFindOpenGL
  --build-options ${build_options}
  -DOpenGL_TEST_VND=${_vnd_testing}
  --test-command ${CMAKE_CTEST_COMMAND} -V -C $<CONFIGURATION>
  )
