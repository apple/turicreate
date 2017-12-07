cmake_policy(SET CMP0057 NEW)

function(run_cpack_test_common_ TEST_NAME types build SUBTEST_SUFFIX source PACKAGING_TYPE)
  if(TEST_TYPE IN_LIST types)
    set(RunCMake_TEST_NO_CLEAN TRUE)
    set(RunCMake_TEST_BINARY_DIR "${RunCMake_BINARY_DIR}/${TEST_NAME}-build")
    set(full_test_name_ "${TEST_NAME}")

    if(SUBTEST_SUFFIX)
      set(RunCMake_TEST_BINARY_DIR "${RunCMake_TEST_BINARY_DIR}-${SUBTEST_SUFFIX}-subtest")
      set(full_test_name_ "${full_test_name_}-${SUBTEST_SUFFIX}-subtest")
    endif()

    string(APPEND full_test_name_ "-${PACKAGING_TYPE}-type")

     # TODO this should be executed only once per ctest run (not per generator)
    file(REMOVE_RECURSE "${RunCMake_TEST_BINARY_DIR}")
    file(MAKE_DIRECTORY "${RunCMake_TEST_BINARY_DIR}")

    if(EXISTS "${RunCMake_SOURCE_DIR}/tests/${TEST_NAME}/${TEST_TYPE}-Prerequirements.cmake")
      include("${RunCMake_SOURCE_DIR}/tests/${TEST_NAME}/${TEST_TYPE}-Prerequirements.cmake")

      set(FOUND_PREREQUIREMENTS false)
      get_test_prerequirements("FOUND_PREREQUIREMENTS" "${config_file}")

      # skip the test if prerequirements are not met
      if(NOT FOUND_PREREQUIREMENTS)
        message(STATUS "${TEST_NAME} - SKIPPED")
        return()
      endif()
    endif()

    # execute cmake
    set(RunCMake_TEST_OPTIONS "-DGENERATOR_TYPE=${TEST_TYPE}"
      "-DRunCMake_TEST_FILE_PREFIX=${TEST_NAME}"
      "-DRunCMake_SUBTEST_SUFFIX=${SUBTEST_SUFFIX}"
      "-DPACKAGING_TYPE=${PACKAGING_TYPE}")
    run_cmake(${full_test_name_})

    # execute optional build step
    if(build)
      run_cmake_command(${full_test_name_}-Build "${CMAKE_COMMAND}" --build "${RunCMake_TEST_BINARY_DIR}")
    endif()

    if(source)
      set(pack_params_ -G ${TEST_TYPE} --config ./CPackSourceConfig.cmake)
      FILE(APPEND ${RunCMake_TEST_BINARY_DIR}/CPackSourceConfig.cmake
        "\nset(CPACK_RPM_SOURCE_PKG_BUILD_PARAMS \"-DRunCMake_TEST:STRING=${full_test_name_} -DRunCMake_TEST_FILE_PREFIX:STRING=${TEST_NAME} -DGENERATOR_TYPE:STRING=${TEST_TYPE}\")")
    else()
      unset(pack_params_)
    endif()

    # execute cpack
    execute_process(
      COMMAND ${CMAKE_CPACK_COMMAND} ${pack_params_}
      WORKING_DIRECTORY "${RunCMake_TEST_BINARY_DIR}"
      RESULT_VARIABLE "result_"
      OUTPUT_FILE "${RunCMake_TEST_BINARY_DIR}/test_output.txt"
      ERROR_FILE "${RunCMake_TEST_BINARY_DIR}/test_error.txt"
      )

    foreach(o out err)
      if(SUBTEST_SUFFIX AND EXISTS ${RunCMake_SOURCE_DIR}/tests/${TEST_NAME}/${TEST_TYPE}-${PACKAGING_TYPE}-${SUBTEST_SUFFIX}-std${o}.txt)
        set(RunCMake-std${o}-file "tests/${TEST_NAME}/${TEST_TYPE}-${PACKAGING_TYPE}-${SUBTEST_SUFFIX}-std${o}.txt")
      elseif(EXISTS ${RunCMake_SOURCE_DIR}/tests/${TEST_NAME}/${TEST_TYPE}-${PACKAGING_TYPE}-std${o}.txt)
        set(RunCMake-std${o}-file "tests/${TEST_NAME}/${TEST_TYPE}-${PACKAGING_TYPE}-std${o}.txt")
      elseif(SUBTEST_SUFFIX AND EXISTS ${RunCMake_SOURCE_DIR}/tests/${TEST_NAME}/${TEST_TYPE}-${SUBTEST_SUFFIX}-std${o}.txt)
        set(RunCMake-std${o}-file "tests/${TEST_NAME}/${TEST_TYPE}-${SUBTEST_SUFFIX}-std${o}.txt")
      elseif(EXISTS ${RunCMake_SOURCE_DIR}/tests/${TEST_NAME}/${TEST_TYPE}-std${o}.txt)
        set(RunCMake-std${o}-file "tests/${TEST_NAME}/${TEST_TYPE}-std${o}.txt")
      elseif(EXISTS ${RunCMake_SOURCE_DIR}/${TEST_TYPE}/default_expected_std${o}.txt)
        set(RunCMake-std${o}-file "${TEST_TYPE}/default_expected_std${o}.txt")
      endif()
    endforeach()

    # verify result
    run_cmake_command(
      ${TEST_TYPE}/${full_test_name_}
      "${CMAKE_COMMAND}"
        -DRunCMake_TEST=${full_test_name_}
        -DRunCMake_TEST_FILE_PREFIX=${TEST_NAME}
        -DRunCMake_SUBTEST_SUFFIX=${SUBTEST_SUFFIX}
        -DGENERATOR_TYPE=${TEST_TYPE}
        -DPACKAGING_TYPE=${PACKAGING_TYPE}
        "-Dsrc_dir=${RunCMake_SOURCE_DIR}"
        "-Dbin_dir=${RunCMake_TEST_BINARY_DIR}"
        "-Dconfig_file=${config_file}"
        -P "${RunCMake_SOURCE_DIR}/VerifyResult.cmake"
      )
  endif()
endfunction()

function(run_cpack_test TEST_NAME types build PACKAGING_TYPES)
  foreach(packaging_type_ IN LISTS PACKAGING_TYPES)
    run_cpack_test_common_("${TEST_NAME}" "${types}" "${build}" "" false "${packaging_type_}")
  endforeach()
endfunction()

function(run_cpack_test_subtests TEST_NAME SUBTEST_SUFFIXES types build PACKAGING_TYPES)
  foreach(suffix_ IN LISTS SUBTEST_SUFFIXES)
    foreach(packaging_type_ IN LISTS PACKAGING_TYPES)
      run_cpack_test_common_("${TEST_NAME}" "${types}" "${build}" "${suffix_}" false "${packaging_type_}")
    endforeach()
  endforeach()
endfunction()

function(run_cpack_source_test TEST_NAME types)
  run_cpack_test_common_("${TEST_NAME}" "${types}" false "" true "")
endfunction()
