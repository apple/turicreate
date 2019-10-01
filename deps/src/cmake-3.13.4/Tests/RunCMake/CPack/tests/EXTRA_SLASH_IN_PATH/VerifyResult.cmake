# check that relocation path is /foo and not //foo
getPackageInfo("${FOUND_FILE_1}" "FILE_INFO_")
set(whitespaces_ "[\t\n\r ]*")
if(NOT FILE_INFO_ MATCHES "Relocations${whitespaces_}:${whitespaces_}/${whitespaces_}/foo")
  message(FATAL_ERROR "Unexpected relocation path in file '${FOUND_FILE_1}';"
    " file info: '${FILE_INFO_}'")
endif()
