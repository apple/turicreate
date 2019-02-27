function(checkPackageInfo_ TYPE FILE REGEX)
  getPackageInfo("${FILE}" "FILE_INFO_")
  if(NOT FILE_INFO_ MATCHES "${REGEX}")
    message(FATAL_ERROR "Unexpected ${TYPE} in '${FILE}' ${EXPECTED_FILE_1_VERSION} ${EXPECTED_FILE_1_REVISION}; file info: '${FILE_INFO_}'")
  endif()
endfunction()

set(whitespaces_ "[\t\n\r ]*")

checkPackageInfo_("version" "${FOUND_FILE_1}"
  ".*Version${whitespaces_}:${whitespaces_}5.0.1-71-g884852e")
