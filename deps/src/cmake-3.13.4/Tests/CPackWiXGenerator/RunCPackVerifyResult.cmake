message(STATUS "=============================================================")
message(STATUS "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)")
message(STATUS "")

if(NOT CPackWiXGenerator_BINARY_DIR)
  message(FATAL_ERROR "CPackWiXGenerator_BINARY_DIR not set")
endif()

message(STATUS "CMAKE_COMMAND: ${CMAKE_COMMAND}")
message(STATUS "CMAKE_CPACK_COMMAND: ${CMAKE_CPACK_COMMAND}")
message(STATUS "CPackWiXGenerator_BINARY_DIR: ${CPackWiXGenerator_BINARY_DIR}")

if(config)
  set(_C_config -C ${config})
endif()

execute_process(COMMAND "${CMAKE_CPACK_COMMAND}"
                        ${_C_config}
  RESULT_VARIABLE CPack_result
  OUTPUT_VARIABLE CPack_output
  ERROR_VARIABLE CPack_error
  WORKING_DIRECTORY "${CPackWiXGenerator_BINARY_DIR}")

if(CPack_result)
  message(FATAL_ERROR "CPack execution went wrong!, CPack_output=${CPack_output}, CPack_error=${CPack_error}")
else ()
  message(STATUS "CPack_output=${CPack_output}")
endif()

set(expected_file_mask "*.msi")
file(GLOB installer_file "${expected_file_mask}")

message(STATUS "installer_file='${installer_file}'")
message(STATUS "expected_file_mask='${expected_file_mask}'")

if(NOT installer_file)
  message(FATAL_ERROR "installer_file does not exist.")
endif()

function(run_wix_command command)
  file(TO_CMAKE_PATH "$ENV{WIX}" WIX_ROOT)
  set(WIX_PROGRAM "${WIX_ROOT}/bin/${command}.exe")

  if(NOT EXISTS "${WIX_PROGRAM}")
    message(FATAL_ERROR "Failed to find WiX Tool: ${WIX_PROGRAM}")
  endif()

  message(STATUS "Running WiX Tool: ${command} ${ARGN}")

  execute_process(COMMAND "${WIX_PROGRAM}" ${ARGN}
    RESULT_VARIABLE WIX_result
    OUTPUT_VARIABLE WIX_output
    ERROR_VARIABLE WIX_output
    WORKING_DIRECTORY "${CPackWiXGenerator_BINARY_DIR}")

  message(STATUS "${command} Output: \n${WIX_output}")

  if(WIX_result)
    message(FATAL_ERROR "WiX ${command} failed: ${WIX_result}")
  endif()
endfunction()

file(GLOB WXS_SOURCE_FILES
  "${CPackWiXGenerator_BINARY_DIR}/_CPack_Packages/*/WIX/*.wxs")

if(NOT WXS_SOURCE_FILES)
  message(FATAL_ERROR "Failed finding WiX source files to validate.")
endif()

foreach(WXS_SOURCE_FILE IN LISTS WXS_SOURCE_FILES)
  run_wix_command(wixcop "${WXS_SOURCE_FILE}")
endforeach()

# error SMOK1076 : ICE61: This product should remove only older
# versions of itself. The Maximum version is not less
# than the current product. (1.0.0 1.0.0)
run_wix_command(smoke -nologo -wx -sw1076 "${installer_file}")
