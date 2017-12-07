cmake_minimum_required(VERSION 3.4)
include(RunCMake)

# Function to build and install a project.  The latter step *-check.cmake
# scripts can check installed files using the check_installed function.
function(run_install_test case)
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/${case}-build)
  set(RunCMake_TEST_NO_CLEAN 1)
  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")
  run_cmake(${case})
  run_cmake_command(${case}-build ${CMAKE_COMMAND} --build . --config Debug)
  # Check "all" components.
  set(CMAKE_INSTALL_PREFIX ${RunCMake_TEST_BINARY_DIR}/root-all)
  run_cmake_command(${case}-all ${CMAKE_COMMAND} -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -DBUILD_TYPE=Debug -P cmake_install.cmake)

  if(run_install_test_components)
    # Check unspecified component.
    set(CMAKE_INSTALL_PREFIX ${RunCMake_TEST_BINARY_DIR}/root-uns)
    run_cmake_command(${case}-uns ${CMAKE_COMMAND} -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -DBUILD_TYPE=Debug -DCOMPONENT=Unspecified -P cmake_install.cmake)
    # Check explicit component.
    set(CMAKE_INSTALL_PREFIX ${RunCMake_TEST_BINARY_DIR}/root-exc)
    run_cmake_command(${case}-exc ${CMAKE_COMMAND} -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -DBUILD_TYPE=Debug -DCOMPONENT=exc -P cmake_install.cmake)
  endif()
endfunction()

# Function called in *-check.cmake scripts to check installed files.
function(check_installed expect)
  file(GLOB_RECURSE actual
    LIST_DIRECTORIES TRUE
    RELATIVE ${CMAKE_INSTALL_PREFIX}
    ${CMAKE_INSTALL_PREFIX}/*
    )
  if(actual)
    list(SORT actual)
  endif()
  if(NOT "${actual}" MATCHES "${expect}")
    set(RunCMake_TEST_FAILED "Installed files:
  ${actual}
do not match what we expected:
  ${expect}
in directory:
  ${CMAKE_INSTALL_PREFIX}" PARENT_SCOPE)
  endif()
endfunction()

run_cmake(DIRECTORY-MESSAGE_NEVER)
run_cmake(DIRECTORY-PATTERN-MESSAGE_NEVER)
run_cmake(DIRECTORY-message)
run_cmake(DIRECTORY-message-lazy)
run_cmake(SkipInstallRulesWarning)
run_cmake(SkipInstallRulesNoWarning1)
run_cmake(SkipInstallRulesNoWarning2)
run_cmake(DIRECTORY-DIRECTORY-bad)
run_cmake(DIRECTORY-DESTINATION-bad)
run_cmake(FILES-DESTINATION-bad)
run_cmake(TARGETS-DESTINATION-bad)
run_cmake(EXPORT-OldIFace)
run_cmake(CMP0062-OLD)
run_cmake(CMP0062-NEW)
run_cmake(CMP0062-WARN)

if(NOT RunCMake_GENERATOR STREQUAL "Xcode" OR NOT "$ENV{CMAKE_OSX_ARCHITECTURES}" MATCHES "[;$]")
  run_install_test(FILES-TARGET_OBJECTS)
endif()

set(run_install_test_components 1)
run_install_test(FILES-EXCLUDE_FROM_ALL)
run_install_test(TARGETS-EXCLUDE_FROM_ALL)
