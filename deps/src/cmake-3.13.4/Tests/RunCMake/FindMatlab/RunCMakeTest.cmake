
include(RunCMake)


if(NOT "${MCR_ROOT}" STREQUAL "")
    if(NOT EXISTS "${MCR_ROOT}")
        message(FATAL_ERROR "MCR does not exist ${MCR_ROOT}")
    endif()
    set(RunCMake_TEST_OPTIONS "-Dmatlab_root=${MCR_ROOT}")
endif()
run_cmake(MatlabTest1)

if(RunCMake_GENERATOR MATCHES "Make" AND UNIX)
  # Use a single build tree for a few tests without cleaning.
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/RerunFindMatlab-build-init)
  set(RunCMake_TEST_NO_CLEAN 1)
  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")

  message(STATUS "RerunFindMatlab: first configuration to extract real Matlab_ROOT_DIR")
  set(RunCMake_TEST_OPTIONS "-Dmatlab_required=REQUIRED")
  if(NOT "${MCR_ROOT}" STREQUAL "")
    set(RunCMake_TEST_OPTIONS ${RunCMake_TEST_OPTIONS} "-Dmatlab_root=${MCR_ROOT}")
  endif()
  run_cmake(MatlabTest2)

  message(STATUS "RerunFindMatlab: flushing the variables")
  execute_process(COMMAND
                    ${CMAKE_COMMAND} -L ${RunCMake_TEST_BINARY_DIR}
                    RESULT_VARIABLE _MatlabTest2_error
                    OUTPUT_VARIABLE _MatlabTest2_output)
  if(NOT _MatlabTest2_error EQUAL 0)
    message(FATAL_ERROR "RerunFindMatlab: cannot list the variables ...")
  endif()

  string(REGEX MATCH "Matlab_ROOT_DIR.+=([^\r\n]+)" _matched ${_MatlabTest2_output})

  set(Matlab_ROOT_DIR_correct "${CMAKE_MATCH_1}")
  if(Matlab_ROOT_DIR_correct STREQUAL "")
    message(FATAL_ERROR "RerunFindMatlab: cannot extract Matlab_ROOT_DIR")
  endif()
  message(STATUS "RerunFindMatlab: detected correct Matlab_ROOT_DIR=${Matlab_ROOT_DIR_correct}")

  message(STATUS "RerunFindMatlab: change configuration, incorrect Matlab_ROOT_DIR setting")
  set(RunCMake_TEST_BINARY_DIR ${RunCMake_BINARY_DIR}/RerunFindMatlab-build-second)
  set(RunCMake_TEST_NO_CLEAN 1)
  file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
  file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")

  set(RunCMake_TEST_OPTIONS "-DMatlab_ROOT_DIR=/" "-Dmatlab_required=")
  run_cmake(MatlabTest2)

  message(STATUS "RerunFindMatlab: fixing configuration with correct Matlab_ROOT_DIR setting")
  set(RunCMake_TEST_OPTIONS "-DMatlab_ROOT_DIR=${Matlab_ROOT_DIR_correct}") # required this time?
  run_cmake(MatlabTest2)

  # no target on this test
  run_cmake_command(MatlabTest2 ${CMAKE_COMMAND} --build .)

  unset(RunCMake_TEST_BINARY_DIR)
  unset(RunCMake_TEST_NO_CLEAN)
endif()
