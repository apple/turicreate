function(checkPackageInfo_ TYPE FILE REGEX)
  getPackageInfo("${FILE}" "FILE_INFO_")
  if(NOT FILE_INFO_ MATCHES "${REGEX}")
    message(FATAL_ERROR "Unexpected ${TYPE} in '${FILE}' ${EXPECTED_FILE_1_VERSION} ${EXPECTED_FILE_1_REVISION}; file info: '${FILE_INFO_}'")
  endif()
endfunction()

set(whitespaces_ "[\t\n\r ]*")

if(GENERATOR_TYPE STREQUAL "RPM")
  checkPackageInfo_("package version" "${FOUND_FILE_1}" "Version${whitespaces_}:${whitespaces_}${EXPECTED_FILE_1_VERSION}")
  checkPackageInfo_("package revision" "${FOUND_FILE_1}" "Release${whitespaces_}:${whitespaces_}${EXPECTED_FILE_1_REVISION}")
  checkPackageInfo_("epoch version" "${FOUND_FILE_1}" "Epoch${whitespaces_}:${whitespaces_}3")
else() # DEB
  checkPackageInfo_("version" "${FOUND_FILE_1}"
    ".*Version${whitespaces_}:${whitespaces_}3:${EXPECTED_FILE_1_VERSION}-${EXPECTED_FILE_1_REVISION}")
endif()
