# prevent older policies from interfearing with this script
cmake_policy(PUSH)
cmake_policy(VERSION ${CMAKE_VERSION})


include(CMakeParseArguments)

message(STATUS "=============================================================================")
message(STATUS "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)")
message(STATUS "")

if(NOT CPackComponentsDEB_BINARY_DIR)
  message(FATAL_ERROR "CPackComponentsDEB_BINARY_DIR not set")
endif()

if(NOT CPackGen)
  message(FATAL_ERROR "CPackGen not set")
endif()

message("CMAKE_CPACK_COMMAND = ${CMAKE_CPACK_COMMAND}")
if(NOT CMAKE_CPACK_COMMAND)
  message(FATAL_ERROR "CMAKE_CPACK_COMMAND not set")
endif()

if(NOT CPackDEBConfiguration)
  message(FATAL_ERROR "CPackDEBConfiguration not set")
endif()

# run cpack with some options and returns the list of files generated
# -output_expected_file: list of files that match the pattern
function(run_cpack output_expected_file CPack_output_parent CPack_error_parent)
  set(options )
  set(oneValueArgs "EXPECTED_FILE_MASK" "CONFIG_VERBOSE")
  set(multiValueArgs "CONFIG_ARGS")
  cmake_parse_arguments(run_cpack_deb "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )


  # clean-up previously CPack generated files
  if(${run_cpack_deb_EXPECTED_FILE_MASK})
    file(GLOB expected_file "${${run_cpack_deb_EXPECTED_FILE_MASK}}")
    if (expected_file)
      file(REMOVE "${expected_file}")
    endif()
  endif()

  message("config_args = ${run_cpack_deb_CONFIG_ARGS}")
  message("config_verbose = ${run_cpack_deb_CONFIG_VERBOSE}")
  execute_process(COMMAND ${CMAKE_CPACK_COMMAND} ${run_cpack_deb_CONFIG_VERBOSE} -G ${CPackGen} ${run_cpack_deb_CONFIG_ARGS}
      RESULT_VARIABLE CPack_result
      OUTPUT_VARIABLE CPack_output
      ERROR_VARIABLE CPack_error
      WORKING_DIRECTORY ${CPackComponentsDEB_BINARY_DIR})

  set(${CPack_output_parent} ${CPack_output} PARENT_SCOPE)
  set(${CPack_error_parent}  ${CPack_error} PARENT_SCOPE)

  if (CPack_result)
    message(FATAL_ERROR "error: CPack execution went wrong!, CPack_output=${CPack_output}, CPack_error=${CPack_error}")
  else ()
    message(STATUS "CPack_output=${CPack_output}")
    message(STATUS "CPack_error=${CPack_error}")
  endif()


  if(run_cpack_deb_EXPECTED_FILE_MASK)
    file(GLOB _output_expected_file "${run_cpack_deb_EXPECTED_FILE_MASK}")
    set(${output_expected_file} "${_output_expected_file}" PARENT_SCOPE)
  endif()
endfunction()



# This function runs lintian on a .deb and returns its output
function(run_lintian lintian_output)
  set(${lintian_output} "" PARENT_SCOPE)

  find_program(LINTIAN_EXECUTABLE lintian)
  if(LINTIAN_EXECUTABLE)
    set(options "")
    set(oneValueArgs "FILENAME")
    set(multiValueArgs "")
    cmake_parse_arguments(run_lintian_deb "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )


    if(NOT run_lintian_deb_FILENAME)
      message(FATAL_ERROR "error: run_lintian needs FILENAME to be set")
    endif()

    # run dpkg-deb
    execute_process(COMMAND ${LINTIAN_EXECUTABLE} ${run_lintian_deb_FILENAME}
      WORKING_DIRECTORY "${CPACK_TEMPORARY_DIRECTORY}"
      OUTPUT_VARIABLE LINTIAN_OUTPUT
      RESULT_VARIABLE LINTIAN_RESULT
      ERROR_VARIABLE LINTIAN_ERROR
      OUTPUT_STRIP_TRAILING_WHITESPACE )

    set(${lintian_output} "${LINTIAN_OUTPUT}" PARENT_SCOPE)
  else()
    message(FATAL_ERROR "run_lintian called without lintian executable being present")
  endif()
endfunction()


# Higher level lintian check that parse the output for errors and required strings
function(lintian_check_specific_errors output_errors)
  set(${output_errors} "" PARENT_SCOPE)
  set(ERROR_ACC)

  set(options "")
  set(oneValueArgs "FILENAME")
  set(multiValueArgs "ERROR_REGEX_STRINGS")
  cmake_parse_arguments(lintian_check_specific_errors_deb "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

  set(lintian_output)
  run_lintian(lintian_output FILENAME ${lintian_check_specific_errors_deb_FILENAME})

  message(STATUS "Lintian output is ''${lintian_output}'")

  # regex to avoid
  foreach(_s IN LISTS lintian_check_specific_errors_deb_ERROR_REGEX_STRINGS)

    if("${_s}" STREQUAL "")
       continue()
    endif()

    string(REGEX MATCHALL "${_s}" "_TMP_CHECK_ERROR" "${lintian_output}")

    if(NOT "${_TMP_CHECK_ERROR}" STREQUAL "")
      string(APPEND ERROR_ACC "\nlintian: ${_f}: output contains an undesirable regex:\n\t${_TMP_CHECK_ERROR}")
    endif()
  endforeach()

  set(${output_errors} "${ERROR_ACC}" PARENT_SCOPE)
endfunction()




# This function runs dpkg-deb on a .deb and returns its output
# the default behaviour it to run "--info" on the specified Debian package
# ACTION is one of the option accepted by dpkg-deb
function(run_dpkgdeb dpkg_deb_output)
  set(${dpkg_deb_output} "" PARENT_SCOPE)

  find_program(DPKGDEB_EXECUTABLE dpkg-deb)
  if(DPKGDEB_EXECUTABLE)

    set(options "")
    set(oneValueArgs "FILENAME" "ACTION")
    set(multiValueArgs "")
    cmake_parse_arguments(run_dpkgdeb_deb "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )


    if(NOT run_dpkgdeb_deb_FILENAME)
      message(FATAL_ERROR "error: run_dpkgdeb needs FILENAME to be set")
    endif()

    if(NOT run_dpkgdeb_deb_ACTION)
      set(run_dpkgdeb_deb_ACTION "--info")
    endif()

    # run dpkg-deb
    execute_process(COMMAND ${DPKGDEB_EXECUTABLE} ${run_dpkgdeb_deb_ACTION} ${run_dpkgdeb_deb_FILENAME}
      WORKING_DIRECTORY "${CPACK_TEMPORARY_DIRECTORY}"
      OUTPUT_VARIABLE DPKGDEB_OUTPUT
      RESULT_VARIABLE DPKGDEB_RESULT
      ERROR_VARIABLE DPKGDEB_ERROR
      OUTPUT_STRIP_TRAILING_WHITESPACE )

    if(NOT ("${DPKGDEB_RESULT}" EQUAL "0"))
      message(FATAL_ERROR "Error '${DPKGDEB_RESULT}' returned by dpkg-deb: '${DPKGDEB_ERROR}'")
    endif()

    set(${dpkg_deb_output} "${DPKGDEB_OUTPUT}" PARENT_SCOPE)
  else()
    message(FATAL_ERROR "run_dpkgdeb called without dpkg-deb executable being present")
  endif()
endfunction()


# returns a particular line of the metadata of the .deb, for checking
# a previous call to run_dpkgdeb should provide the DPKGDEB_OUTPUT entry.
function(dpkgdeb_return_specific_metaentry output)
  set(${output} "" PARENT_SCOPE)

  set(options "")
  set(oneValueArgs "DPKGDEB_OUTPUT" "METAENTRY")
  set(multiValueArgs "")
  cmake_parse_arguments(dpkgdeb_return_specific_metaentry_deb "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

  if(NOT dpkgdeb_return_specific_metaentry_deb_METAENTRY)
    message(FATAL_ERROR "error: dpkgdeb_return_specific_metaentry needs METAENTRY to be set")
  endif()

  string(REGEX MATCH "${dpkgdeb_return_specific_metaentry_deb_METAENTRY}([^\r\n]*)" _TMP_STR "${dpkgdeb_return_specific_metaentry_deb_DPKGDEB_OUTPUT}")
  #message("################ _TMP_STR = ${CMAKE_MATCH_1} ##################")
  if(NOT "${CMAKE_MATCH_1}" STREQUAL "")
    string(STRIP "${CMAKE_MATCH_1}" _TMP_STR)
    set(${output} "${_TMP_STR}" PARENT_SCOPE)
  endif()
endfunction()

cmake_policy(POP)
