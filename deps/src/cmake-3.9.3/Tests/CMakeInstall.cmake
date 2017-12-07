# Define option CMake_TEST_INSTALL, and enable by default for dashboards.
set(_default 0)
if(DEFINED ENV{DASHBOARD_TEST_FROM_CTEST})
  set(_default 1)
endif()
option(CMake_TEST_INSTALL "Test CMake Installation" ${_default})
mark_as_advanced(CMake_TEST_INSTALL)

if(CMake_TEST_INSTALL)
  # Do not build during the test.
  set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY 1)

  # Install to a test directory.
  set(CMake_TEST_INSTALL_PREFIX ${CMake_BINARY_DIR}/Tests/CMakeInstall)
  set(CMAKE_INSTALL_PREFIX "${CMake_TEST_INSTALL_PREFIX}")

  if(CMAKE_CONFIGURATION_TYPES)
    # There are multiple configurations.  Make sure the tested
    # configuration is the one that is installed.
    set(CMake_TEST_INSTALL_CONFIG --config $<CONFIGURATION>)
  else()
    set(CMake_TEST_INSTALL_CONFIG)
  endif()

  # Add a test to install CMake through the build system install target.
  add_test(NAME CMake.Install
    COMMAND cmake --build . --target install ${CMake_TEST_INSTALL_CONFIG}
    )

  # Avoid running this test simultaneously with other tests:
  set_tests_properties(CMake.Install PROPERTIES RUN_SERIAL ON)

  # TODO: Make all other tests depend on this one, and then drive them
  # with the installed CTest.
else()
  set(CMake_TEST_INSTALL_PREFIX)
endif()
