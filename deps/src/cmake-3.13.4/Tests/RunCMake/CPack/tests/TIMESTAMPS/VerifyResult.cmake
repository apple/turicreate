macro(getFileMetadata_ FILE RESULT_VAR)
  if(GENERATOR_TYPE STREQUAL "TGZ")
    # getPackageContent defined for archives omit the metadata (non-verbose)
    execute_process(COMMAND ${CMAKE_COMMAND} -E env TZ=Etc/UTC ${CMAKE_COMMAND} -E tar -xtvf ${FILE}
            OUTPUT_VARIABLE ${RESULT_VAR}
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE)
  else()
    getPackageContent("${FILE}" ${RESULT_VAR})
  endif()
endmacro()

function(checkContentTimestamp FILE REGEX)
  getFileMetadata_("${FILE}" METADATA_)

  if(NOT METADATA_ MATCHES "${REGEX}")
    string(REPLACE "\n" "\n  " metadata_indented "${METADATA_}")
    message(FATAL_ERROR
      "Wrong timestamps in file:\n"
      "  ${FILE}\n"
      "Expected timestamps to match:\n"
      "  ${REGEX}\n"
      "Actual timestamps:\n"
      "  ${metadata_indented}")
  endif()
endfunction()

function(checkTimestamp FILE_NAME)
  file(READ ${FILE_NAME} ACTUAL_TIMESTAMP OFFSET 4 LIMIT 4 HEX)

  if(NOT ACTUAL_TIMESTAMP STREQUAL "00000000")
    message(FATAL_ERROR "${FILE_NAME} contains a timestamp [0x${ACTUAL_TIMESTAMP}]")
  endif()
endfunction()

# Expected timestamp is UNIX time 123456789
if(GENERATOR_TYPE STREQUAL "TGZ")
  set(EXPECTED_TIMESTAMP "29 Nov +1973")
  set(EXPECTED_FILES foo/ foo/CMakeLists.txt)
else()
  set(EXPECTED_TIMESTAMP "1973-11-29 21:33")
  set(EXPECTED_FILES ./usr/ ./usr/foo/ ./usr/foo/CMakeLists.txt)
endif()

set(EXPECTED_METADATA)
foreach(FILE ${EXPECTED_FILES})
  list(APPEND EXPECTED_METADATA ".* ${EXPECTED_TIMESTAMP} ${FILE}")
endforeach()
list(JOIN EXPECTED_METADATA ".*" EXPECTED_REGEX)
checkContentTimestamp("${FOUND_FILE_1}" "${EXPECTED_REGEX}")

if(GENERATOR_TYPE STREQUAL "TGZ")
  checkTimestamp("${FOUND_FILE_1}")
else()
  execute_process(COMMAND ${CMAKE_COMMAND} -E tar xf "${FOUND_FILE_1}")
  checkTimestamp("data.tar.gz")
  checkTimestamp("control.tar.gz")
endif()
